/*
 *  Copyright (C) 2015-2020 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <dlfcn.h>

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <vector>

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/include/module.h>
#include <viua/kernel/kernel.h>
#include <viua/loader.h>
#include <viua/machine.h>
#include <viua/runtime/imports.h>
#include <viua/scheduler/ffi.h>
#include <viua/scheduler/io.h>
#include <viua/scheduler/process.h>
#include <viua/support/env.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/types/exception.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/io.h>
#include <viua/types/reference.h>
#include <viua/types/string.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>


viua::kernel::Mailbox::Mailbox(Mailbox&& that)
        : messages(std::move(that.messages))
{}

auto viua::kernel::Mailbox::send(std::unique_ptr<viua::types::Value> message)
    -> void
{
    std::unique_lock<std::mutex> lck{mailbox_mutex};
    messages.push_back(std::move(message));
}

auto viua::kernel::Mailbox::receive(
    std::queue<std::unique_ptr<viua::types::Value>>& mq) -> void
{
    std::unique_lock<std::mutex> lck{mailbox_mutex};
    for (auto& message : messages) {
        mq.push(std::move(message));
    }
    messages.clear();
}

auto viua::kernel::Mailbox::size() const -> decltype(messages)::size_type
{
    std::unique_lock<std::mutex> lck{mailbox_mutex};
    return messages.size();
}


viua::kernel::Process_result::Process_result(Process_result&& that)
{
    value_returned   = std::move(that.value_returned);
    exception_thrown = std::move(that.exception_thrown);
    done.store(that.done.load(std::memory_order_acquire),
               std::memory_order_release);
}
auto viua::kernel::Process_result::resolve(
    std::unique_ptr<viua::types::Value> result) -> void
{
    std::unique_lock<std::mutex> lck{result_mutex};
    value_returned = std::move(result);
    done.store(true, std::memory_order_release);
}
auto viua::kernel::Process_result::raise(
    std::unique_ptr<viua::types::Value> failure) -> void
{
    std::unique_lock<std::mutex> lck{result_mutex};
    exception_thrown = std::move(failure);
    done.store(true, std::memory_order_release);
}
auto viua::kernel::Process_result::stopped() const -> bool
{
    return done.load(std::memory_order_acquire);
}
auto viua::kernel::Process_result::terminated() const -> bool
{
    if (done.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lck{result_mutex};
        return static_cast<bool>(exception_thrown);
    }
    return false;
}
auto viua::kernel::Process_result::transfer_exception()
    -> std::unique_ptr<viua::types::Value>
{
    std::unique_lock<std::mutex> lck{result_mutex};
    return std::move(exception_thrown);
}
auto viua::kernel::Process_result::transfer_result()
    -> std::unique_ptr<viua::types::Value>
{
    std::unique_lock<std::mutex> lck{result_mutex};
    return std::move(value_returned);
}


viua::kernel::Kernel& viua::kernel::Kernel::load(std::unique_ptr<uint8_t[]> bc)
{
    /*  Load bytecode into the viua::kernel::Kernel.
     *  viua::kernel::Kernel becomes owner of loaded bytecode - meaning it will
     * consider itself responsible for proper destruction of it, so make sure
     * you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call
     * .load(nullptr).
     */
    bytecode = std::move(bc);
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::bytes(
    viua::bytecode::codec::bytecode_size_type sz)
{
    /*  Set bytecode size, so the viua::kernel::Kernel can stop execution even
     * if it doesn't reach HALT instruction but reaches bytecode address out of
     * bounds.
     */
    bytecode_size = sz;
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::mapfunction(
    std::string const& name,
    viua::bytecode::codec::bytecode_size_type address)
{
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::mapblock(
    std::string const& name,
    viua::bytecode::codec::bytecode_size_type address)
{
    /** Maps block name to bytecode address.
     */
    block_addresses[name] = address;
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::register_external_function(
    std::string const& name,
    ForeignFunction* function_ptr)
{
    /** Registers external function in viua::kernel::Kernel.
     */
    std::unique_lock<std::mutex> lock(foreign_functions_mutex);
    foreign_functions[name] = function_ptr;
    return (*this);
}


void viua::kernel::Kernel::load_module(std::string module)
{
    auto const module_path = viua::runtime::imports::find_module(module);
    if (not module_path.has_value()) {
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"LinkException"},
            ("failed to locate module: " + module));
    }

    switch (module_path->first) {
    case viua::runtime::imports::Module_type::Native:
        load_native_module(module, module_path->second);
        break;
    case viua::runtime::imports::Module_type::Bytecode:
        load_bytecode_module(module, module_path->second);
        break;
    default:
        assert(false);
    }
}
void viua::kernel::Kernel::load_bytecode_module(
    std::string_view const module_name,
    std::string const& module_path)
{
    Loader loader(module_path);
    loader.load();

    std::unique_ptr<uint8_t[]> lnk_btcd{loader.get_bytecode()};

    auto const fn_names = loader.get_functions();
    auto const fn_addrs = loader.get_function_addresses();
    for (auto const& fn_linkname : fn_names) {
        linked_functions[fn_linkname] = std::pair<std::string, uint8_t*>(
            module_name, (lnk_btcd.get() + fn_addrs.at(fn_linkname)));
    }

    auto const bl_names = loader.get_blocks();
    auto const bl_addrs = loader.get_block_addresses();
    for (auto const& bl_linkname : bl_names) {
        linked_blocks[bl_linkname] = std::pair<std::string, uint8_t*>(
            module_name, (lnk_btcd.get() + bl_addrs.at(bl_linkname)));
    }

    linked_modules[std::string{module_name}] =
        std::pair<viua::bytecode::codec::bytecode_size_type,
                  std::unique_ptr<uint8_t[]>>(loader.get_bytecode_size(),
                                              std::move(lnk_btcd));
}
void viua::kernel::Kernel::load_native_module(
    std::string_view const module_name,
    std::string const& module_path)
{
    void* handle = dlopen(module_path.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if (handle == nullptr) {
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"LinkException"},
            ("failed to open handle: " + std::string{module_name} + ": "
             + dlerror()));
    }

    using ExporterFunction   = const Foreign_function_spec* (*)();
    ExporterFunction exports = nullptr;
    if ((exports = reinterpret_cast<ExporterFunction>(dlsym(handle, "exports")))
        == nullptr) {
        throw std::make_unique<viua::types::Exception>(
            "failed to extract interface from module: "
            + std::string{module_name});
    }

    const Foreign_function_spec* exported = (*exports)();

    size_t i = 0;
    while (exported[i].name != nullptr) {
        register_external_function(exported[i].name, exported[i].fpointer);
        ++i;
    }

    cxx_dynamic_lib_handles.push_back(handle);
}
auto viua::kernel::Kernel::module_at(uint8_t const* const location) const
    -> std::optional<std::string>
{
    if (bytecode.get() == location) {
        return commandline_arguments.at(0);
    }
    for (auto const& [name, mod] : linked_modules) {
        if (mod.second.get() == location) {
            return name;
        }
    }
    return {};
}
auto viua::kernel::Kernel::in_which_function(std::string const mod,
                                             uint64_t const offset) const
    -> std::optional<std::string>
{
    auto const main_module = (mod == commandline_arguments.at(0));

    auto const jump_base = main_module ? bytecode.get()
                                       : linked_modules.at(mod).second.get();

    auto fns = std::map<std::string, uint64_t>{};
    if (main_module) {
        fns = function_addresses;
    } else {
        for (auto const& [fn, spec] : linked_functions) {
            if (spec.first != mod) {
                continue;
            }
            fns[fn] = static_cast<uint64_t>(spec.second - jump_base);
        }
    }

    auto const mod_size = main_module ? (bytecode_size - executable_offset)
                                      : linked_modules.at(mod).first;
    if (offset >= mod_size) {
        return "<outside of executable range>";
    }

    auto candidate_name = std::string{};
    auto candidate_addr = uint64_t{};
    for (auto const& [fn, relative_addr] : fns) {
        if (offset == relative_addr) {
            return fn;
        }
        if (offset > relative_addr and relative_addr > candidate_addr) {
            candidate_name = fn;
            candidate_addr = relative_addr;
        }
    }

    return candidate_name;
}

bool viua::kernel::Kernel::is_local_function(std::string const& name) const
{
    return function_addresses.count(name);
}

bool viua::kernel::Kernel::is_linked_function(std::string const& name) const
{
    return linked_functions.count(name);
}

bool viua::kernel::Kernel::is_native_function(std::string const& name) const
{
    return (function_addresses.count(name) or linked_functions.count(name));
}

bool viua::kernel::Kernel::is_foreign_function(std::string const& name) const
{
    return foreign_functions.count(name);
}

bool viua::kernel::Kernel::is_block(std::string const& name) const
{
    return (block_addresses.count(name) or linked_blocks.count(name));
}

bool viua::kernel::Kernel::is_local_block(std::string const& name) const
{
    return block_addresses.count(name);
}

bool viua::kernel::Kernel::is_linked_block(std::string const& name) const
{
    return linked_blocks.count(name);
}

auto viua::kernel::Kernel::get_entry_point_of_block(std::string const& name)
    const -> std::pair<viua::internals::types::Op_address_type,
                       viua::internals::types::Op_address_type>
{
    auto entry_point = viua::internals::types::Op_address_type{nullptr};
    auto module_base = viua::internals::types::Op_address_type{nullptr};
    if (block_addresses.count(name)) {
        entry_point = (bytecode.get() + block_addresses.at(name));
        module_base = bytecode.get();
    } else {
        auto const lf = linked_blocks.at(name);
        entry_point   = lf.second;
        module_base   = linked_modules.at(lf.first).second.get();
    }
    return std::pair<viua::internals::types::Op_address_type,
                     viua::internals::types::Op_address_type>(entry_point,
                                                              module_base);
}

auto viua::kernel::Kernel::get_entry_point_of(std::string const& name) const
    -> std::pair<viua::internals::types::Op_address_type,
                 viua::internals::types::Op_address_type>
{
    auto entry_point = viua::internals::types::Op_address_type{nullptr};
    auto module_base = viua::internals::types::Op_address_type{nullptr};
    if (function_addresses.count(name)) {
        entry_point = (bytecode.get() + function_addresses.at(name));
        module_base = bytecode.get();
    } else {
        auto const lf = linked_functions.at(name);
        entry_point   = lf.second;
        module_base   = linked_modules.at(lf.first).second.get();
    }
    return std::pair<viua::internals::types::Op_address_type,
                     viua::internals::types::Op_address_type>(entry_point,
                                                              module_base);
}

void viua::kernel::Kernel::request_foreign_function_call(
    std::unique_ptr<Frame> frame,
    viua::process::Process& requesting_process)
{
    std::unique_lock<std::mutex> lock(foreign_call_queue_mutex);
    foreign_call_queue.emplace(
        std::make_unique<viua::scheduler::ffi::Foreign_function_call_request>(
            std::move(frame), requesting_process, *this));

    // unlock before calling notify_one() to avoid waking the worker thread when
    // it cannot obtain the lock and fetch the call request
    lock.unlock();
    foreign_call_queue_condition.notify_one();
}

auto viua::kernel::Kernel::steal_processes()
    -> std::vector<std::unique_ptr<viua::process::Process>>
{
    std::unique_lock<std::mutex> lock{process_spawned_mtx};

    /*
     * If any scheduler spawned a process recently, let's try to steal some work
     * from it.
     */
    if (not process_spawned_by.empty()) {
        auto sched = std::move(process_spawned_by.front());
        process_spawned_by.pop_front();
        return sched->give_up_processes();
    }

    /*
     * If no scheduler spawned a process recently, let's wait for a bit and
     * check again.
     */
    auto const STEAL_WAIT_PERIOD = std::chrono::milliseconds{16};
    process_spawned_cv.wait_for(lock, STEAL_WAIT_PERIOD);

    /*
     * Make sure that we don't accidentaly try to use empty std::queue.
     */
    if (not process_spawned_by.empty()) {
        auto sched = std::move(process_spawned_by.front());
        process_spawned_by.pop_front();
        return sched->give_up_processes();
    }

    auto stolen = std::vector<std::unique_ptr<viua::process::Process>>{};
    return stolen;
}
auto viua::kernel::Kernel::notify_about_process_spawned(
    viua::scheduler::Process_scheduler* sched) -> void
{
    {
        std::unique_lock<std::mutex> lck{process_spawned_mtx};
        process_spawned_by.push_back(sched);
    }
    ++running_processes;
    process_spawned_cv.notify_one();
}
auto viua::kernel::Kernel::notify_about_process_death() -> void
{
    --running_processes;
}
auto viua::kernel::Kernel::process_count() const -> size_t
{
    return running_processes;
}

auto viua::kernel::Kernel::make_pid() -> viua::process::PID
{
    std::unique_lock<std::mutex> lck{pid_mutex};
    return viua::process::PID{pid_sequence.emit()};
}

auto viua::kernel::Kernel::create_mailbox(const viua::process::PID pid)
    -> size_t
{
    std::unique_lock<std::mutex> lck(mailbox_mutex);
#if VIUA_VM_DEBUG_LOG
    std::cerr << "[kernel:mailbox:create] pid = " << pid.get() << std::endl;
#endif
    mailboxes.emplace(pid, Mailbox{});
    return ++running_processes;
}
auto viua::kernel::Kernel::delete_mailbox(const viua::process::PID pid)
    -> size_t
{
    std::unique_lock<std::mutex> lck(mailbox_mutex);
#if VIUA_VM_DEBUG_LOG
    std::cerr << "[kernel:mailbox:delete] pid = " << pid.get()
              << ", queued messages = " << mailboxes[pid].size() << std::endl;
#endif
    mailboxes.erase(pid);
    return --running_processes;
}
auto viua::kernel::Kernel::create_result_slot_for(viua::process::PID pid)
    -> void
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    process_results.emplace(pid, Process_result{});
}
auto viua::kernel::Kernel::detach_process(const viua::process::PID pid) -> void
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    process_results.erase(pid);
}
auto viua::kernel::Kernel::record_process_result(
    viua::process::Process* done_process) -> void
{
    std::unique_lock<std::mutex> lck{process_results_mutex};

    if (process_results.count(done_process->pid()) == 0) {
        return;
    }

    if (process_results.at(done_process->pid()).stopped()) {
        /*
         * FIXME a process cannot return twice
         */
        return;
    }

    if (done_process->terminated()) {
        process_results.at(done_process->pid())
            .raise(done_process->transfer_active_exception());
    } else {
        process_results.at(done_process->pid())
            .resolve(done_process->get_return_value());
    }
}
auto viua::kernel::Kernel::is_process_joinable(
    const viua::process::PID pid) const -> bool
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    return process_results.count(pid);
}
auto viua::kernel::Kernel::is_process_stopped(
    const viua::process::PID pid) const -> bool
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    return process_results.at(pid).stopped();
}
auto viua::kernel::Kernel::is_process_terminated(
    const viua::process::PID pid) const -> bool
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    return process_results.at(pid).terminated();
}
auto viua::kernel::Kernel::transfer_exception_of(const viua::process::PID pid)
    -> std::unique_ptr<viua::types::Value>
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    auto tmp = process_results.at(pid).transfer_exception();
    process_results.erase(pid);
    return tmp;
}
auto viua::kernel::Kernel::transfer_result_of(const viua::process::PID pid)
    -> std::unique_ptr<viua::types::Value>
{
    std::unique_lock<std::mutex> lck{process_results_mutex};
    auto tmp = process_results.at(pid).transfer_result();
    process_results.erase(pid);
    return tmp;
}

void viua::kernel::Kernel::send(const viua::process::PID pid,
                                std::unique_ptr<viua::types::Value> message)
{
    std::unique_lock<std::mutex> lck(mailbox_mutex);
    if (mailboxes.count(pid) == 0) {
        // sending a message to an unknown address just drops the message
        // instead of crashing the sending process
        return;
    }
#if VIUA_VM_DEBUG_LOG
    cerr << "[kernel:receive:send] pid = " << pid.get()
         << ", queued messages = " << mailboxes[pid].size() << "+1" << endl;
#endif
    mailboxes[pid].send(std::move(message));
}
void viua::kernel::Kernel::receive(
    const viua::process::PID pid,
    std::queue<std::unique_ptr<viua::types::Value>>& message_queue)
{
    std::unique_lock<std::mutex> lck(mailbox_mutex);
    if (mailboxes.count(pid) == 0) {
        throw std::make_unique<viua::types::Exception>("invalid PID");
    }

#if VIUA_VM_DEBUG_LOG
    cerr << "[kernel:receive:pre] pid = " << pid.get()
         << ", queued messages = " << message_queue.size() << endl;
#endif
    mailboxes[pid].receive(message_queue);
#if VIUA_VM_DEBUG_LOG
    cerr << "[kernel:receive:post] pid = " << pid.get()
         << ", queued messages = " << message_queue.size() << endl;
#endif
}

uint64_t viua::kernel::Kernel::pids() const
{
    return running_processes;
}

auto viua::kernel::Kernel::schedule_io(
    std::unique_ptr<viua::scheduler::io::IO_interaction> i) -> void
{
    {
        std::unique_lock<std::mutex> lck{io_requests_mtx};
        io_requests.insert({i->id(), i.get()});
    }
    {
        std::unique_lock<std::mutex> lck{io_request_mutex};
        io_request_queue.push_back(std::move(i));
    }
    io_request_cv.notify_one();
}
auto viua::kernel::Kernel::cancel_io(
    std::tuple<uint64_t, uint64_t> const interaction_id) -> void
{
    std::unique_lock<std::mutex> lck{io_requests_mtx};
    /*
     * We have to check if the request is still there to avoid crashing when the
     * cancellation order arrives just after the request has been completed, as
     * request slots are removed after completion.
     */
    // FIXME Maybe change the I/O pipeline so that the .count() call is not
    // needed? A better correctness guarantee would be much more reasurring than
    // a "see if it's there" bandaid. Maybe don't erase the request slot until
    // after the process has waited for the request in question?
    if (io_requests.count(interaction_id)) {
        io_requests.at(interaction_id)->cancel();
    }
}
auto viua::kernel::Kernel::io_complete(
    std::tuple<uint64_t, uint64_t> const interaction_id) const -> bool
{
    std::unique_lock<std::mutex> lck{io_result_mtx};
    if (not io_results.count(interaction_id)) {
        return false;
    }
    return io_results.at(interaction_id).is_complete;
}
auto viua::kernel::Kernel::complete_io(
    std::tuple<uint64_t, uint64_t> const interaction_id,
    IO_result result) -> void
{
    {
        std::unique_lock<std::mutex> lck{io_requests_mtx};
        io_requests.erase(io_requests.find(interaction_id));
    }
    std::unique_lock<std::mutex> lck{io_result_mtx};
    io_results.insert({interaction_id, std::move(result)});
}
auto viua::kernel::Kernel::io_result(
    std::tuple<uint64_t, uint64_t> const interaction_id)
    -> std::unique_ptr<viua::types::Value>
{
    std::unique_lock<std::mutex> lck{io_result_mtx};
    auto result = std::move(io_results.at(interaction_id));
    io_results.erase(io_results.find(interaction_id));
    lck.unlock();

    if (result.is_successful) {
        return std::move(result.value);
    }
    throw std::move(result.error);
}

viua::kernel::Kernel::IO_result::IO_result(
    bool const ok,
    std::unique_ptr<viua::types::Value> x)
        : value{ok ? std::move(x) : nullptr}
        , error{ok ? nullptr : std::move(x)}
        , is_complete{true}
        , is_cancelled{false}
        , is_successful{ok}
{}
auto viua::kernel::Kernel::IO_result::make_success(
    std::unique_ptr<viua::types::Value> x) -> IO_result
{
    return IO_result{true, std::move(x)};
}
auto viua::kernel::Kernel::IO_result::make_error(
    std::unique_ptr<viua::types::Value> x) -> IO_result
{
    return IO_result{false, std::move(x)};
}

int viua::kernel::Kernel::exit() const
{
    return return_code;
}

static auto no_of_schedulers(std::string const env_name,
                             size_t const default_limit) -> size_t
{
    if (auto const env_limit = getenv(env_name.c_str()); env_limit != nullptr) {
        auto const raw_limit = std::stoi(env_limit);
        if (raw_limit > 0) {
            return static_cast<size_t>(raw_limit);
        }
    }
    return default_limit;
}
auto viua::kernel::Kernel::no_of_process_schedulers() -> size_t
{
    auto const default_value = std::thread::hardware_concurrency();
    return no_of_schedulers("VIUA_PROC_SCHEDULERS",
                            static_cast<size_t>(default_value));
}
auto viua::kernel::Kernel::no_of_ffi_schedulers() -> size_t
{
    auto const default_value = (std::thread::hardware_concurrency() / 2);
    return no_of_schedulers("VIUA_FFI_SCHEDULERS",
                            static_cast<size_t>(default_value));
}
auto viua::kernel::Kernel::no_of_io_schedulers() -> size_t
{
    auto const default_value = (std::thread::hardware_concurrency() / 2);
    return no_of_schedulers("VIUA_IO_SCHEDULERS",
                            static_cast<size_t>(default_value));
}
auto viua::kernel::Kernel::is_tracing_enabled() -> bool
{
    auto viua_enable_tracing = std::string{};
    char* env_text           = getenv("VIUA_ENABLE_TRACING");
    if (env_text) {
        viua_enable_tracing = std::string(env_text);
    }
    return (viua_enable_tracing == "yes" or viua_enable_tracing == "true"
            or viua_enable_tracing == "1");
}

int viua::kernel::Kernel::run()
{
    /*  VM viua::kernel::Kernel implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    constexpr auto const KERNEL_SETUP_DEBUG = false;

    auto const vp_schedulers_limit = no_of_process_schedulers();
    if constexpr (KERNEL_SETUP_DEBUG) {
        std::cerr << "[kernel] process scheduler limit: " << vp_schedulers_limit
                  << "\n";
    }
    /* bool enable_tracing = is_tracing_enabled(); */

    process_schedulers.reserve(vp_schedulers_limit);

    /*
     * Immediately allocate and bootstrap the "main" scheduler.
     *
     * It is done outside any loops and limits because we always need at least
     * one scheduler to run any code at all. This way, even if the limit of
     * schedulers is specified to be 0 the VM will be able to run.
     *
     * But why bootstrap it now? Because we need the process to be ready to run
     * immediately after launch. Otherwise it will just shut down because the
     * kernel will tell it that there are no processes in the whole VM and it is
     * free to stop running.
     */
    if constexpr (KERNEL_SETUP_DEBUG) {
        std::cerr << "[kernel] bootstrapping main scheduler\n";
    }
    process_schedulers.emplace_back(
        std::make_unique<viua::scheduler::Process_scheduler>(*this, 0));
    process_schedulers.front()->bootstrap(commandline_arguments);

    /*
     * Allocate the additional process schedulers requested. Remember to launch
     * (n - 1) of them as the main scheduler also counts into the limit of
     * schedulers.
     */
    if (vp_schedulers_limit) {
        auto n = viua::scheduler::Process_scheduler::id_type{1};
        for (auto i = (vp_schedulers_limit - 1); i; --i) {
            process_schedulers.emplace_back(
                std::make_unique<viua::scheduler::Process_scheduler>(*this,
                                                                     n++));
        }
    }
    if constexpr (KERNEL_SETUP_DEBUG) {
        std::cerr << "[kernel] created " << process_schedulers.size()
                  << " process scheduler(s)\n";
    }

    /*
     * Launch all the schedulers. With the main scheduler bootstrapped and all
     * the extra process schedulers already allocated we can just set them loose
     * and let them run the software...
     */
    for (auto& sched : process_schedulers) {
        sched->launch();
    }
    if constexpr (KERNEL_SETUP_DEBUG) {
        std::cerr << "[kernel] all " << process_schedulers.size()
                  << " scheduler(s) launched\n";
    }

    /*
     * ...and then there is nothing for us to do but wait for them to complete
     * running. Please not that this condition may never be met if the software
     * loaded into the VM is intended to run indefinitely.
     */
    for (auto& sched : process_schedulers) {
        sched->shutdown();
        sched->join();
    }
    if constexpr (KERNEL_SETUP_DEBUG) {
        std::cerr << "[kernel] all schedulers shut down\n";
    }

    return_code = process_schedulers.front()->exit();

    return return_code;
}

viua::kernel::Kernel::Kernel()
        : bytecode(nullptr)
        , bytecode_size(0)
        , executable_offset(0)
        , return_code(0)
        , debug(false)
        , errors(false)
{
    auto const ffi_schedulers_limit = no_of_ffi_schedulers();
    for (auto i = ffi_schedulers_limit; i; --i) {
        foreign_call_workers.emplace_back(std::make_unique<std::thread>(
            viua::scheduler::ffi::ff_call_processor,
            &foreign_call_queue,
            &foreign_functions,
            &foreign_functions_mutex,
            &foreign_call_queue_mutex,
            &foreign_call_queue_condition));
        pthread_setname_np(
            foreign_call_workers.back()->native_handle(),
            ("ffi." + std::to_string(ffi_schedulers_limit - i)).c_str());
    }

    auto const io_schedulers_limit = no_of_io_schedulers();
    for (auto i = io_schedulers_limit; i; --i) {
        io_workers.emplace_back(
            std::make_unique<std::thread>(viua::scheduler::io::io_scheduler,
                                          (io_schedulers_limit - i),
                                          std::ref(*this),
                                          std::ref(io_request_queue),
                                          std::ref(io_request_mutex),
                                          std::ref(io_request_cv)));
    }
}

viua::kernel::Kernel::~Kernel()
{
    {
        /*
         * Send a poison pill to every foreign function call worker thread.
         * Collect them after they are killed.
         */
        std::unique_lock<std::mutex> lck{foreign_call_queue_mutex,
                                         std::defer_lock};
        for (auto i = foreign_call_workers.size(); i; --i) {
            lck.lock();

            /*
             * Send the actual poison pill. One per worker thread since we can
             * be sure that a thread consumes at most one pill.
             */
            foreign_call_queue.push(nullptr);

            lck.unlock();
            foreign_call_queue_condition.notify_all();
        }
        while (not foreign_call_workers.empty()) {
            auto w = std::move(foreign_call_workers.back());
            foreign_call_workers.pop_back();
            w->join();
        }
    }
    {
        /*
         * Send a poison pill to every I/O worker thread.
         */
        if constexpr ((false)) {
            std::cerr << "[kernel] starting I/O shutdown, " << io_workers.size()
                      << " workers to kill\n";
        }
        {
            std::unique_lock<std::mutex> lck{io_request_mutex};
            for (auto const& _ [[maybe_unused]] : io_workers) {
                io_request_queue.push_back(nullptr);
            }
        }
        io_request_cv.notify_all();
        for (auto& each : io_workers) {
            if constexpr ((false)) {
                std::cerr << "[kernel] waiting for I/O worker\n";
            }
            each->join();
        }
        if constexpr ((false)) {
            std::cerr << "[kernel] done with I/O shutdown\n";
        }
    }

    for (auto const each : cxx_dynamic_lib_handles) {
        dlclose(each);
    }
}
