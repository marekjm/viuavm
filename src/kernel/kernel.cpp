/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

#include <chrono>
#include <cstdlib>
#include <dlfcn.h>
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
#include <viua/scheduler/ffi.h>
#include <viua/scheduler/vps.h>
#include <viua/support/env.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/types/exception.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/object.h>
#include <viua/types/reference.h>
#include <viua/types/string.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
using namespace std;


viua::kernel::Mailbox::Mailbox(Mailbox&& that)
        : messages(std::move(that.messages)) {}

auto viua::kernel::Mailbox::send(unique_ptr<viua::types::Value> message)
    -> void {
    unique_lock<mutex> lck{mailbox_mutex};
    messages.push_back(std::move(message));
}

auto viua::kernel::Mailbox::receive(queue<unique_ptr<viua::types::Value>>& mq)
    -> void {
    unique_lock<mutex> lck{mailbox_mutex};
    for (auto& message : messages) {
        mq.push(std::move(message));
    }
    messages.clear();
}

auto viua::kernel::Mailbox::size() const -> decltype(messages)::size_type {
    unique_lock<mutex> lck{mailbox_mutex};
    return messages.size();
}


viua::kernel::ProcessResult::ProcessResult(ProcessResult&& that) {
    value_returned   = std::move(that.value_returned);
    exception_thrown = std::move(that.exception_thrown);
    done.store(that.done.load(std::memory_order_acquire),
               std::memory_order_release);
}
auto viua::kernel::ProcessResult::resolve(unique_ptr<viua::types::Value> result)
    -> void {
    unique_lock<mutex> lck{result_mutex};
    value_returned = std::move(result);
    done.store(true, std::memory_order_release);
}
auto viua::kernel::ProcessResult::raise(unique_ptr<viua::types::Value> failure)
    -> void {
    unique_lock<mutex> lck{result_mutex};
    exception_thrown = std::move(failure);
    done.store(true, std::memory_order_release);
}
auto viua::kernel::ProcessResult::stopped() const -> bool {
    return done.load(std::memory_order_acquire);
}
auto viua::kernel::ProcessResult::terminated() const -> bool {
    if (done.load(std::memory_order_acquire)) {
        unique_lock<mutex> lck{result_mutex};
        return static_cast<bool>(exception_thrown);
    }
    return false;
}
auto viua::kernel::ProcessResult::transfer_exception()
    -> unique_ptr<viua::types::Value> {
    unique_lock<mutex> lck{result_mutex};
    return std::move(exception_thrown);
}
auto viua::kernel::ProcessResult::transfer_result()
    -> unique_ptr<viua::types::Value> {
    unique_lock<mutex> lck{result_mutex};
    return std::move(value_returned);
}


viua::kernel::Kernel& viua::kernel::Kernel::load(
    unique_ptr<viua::internals::types::byte[]> bc) {
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
    viua::internals::types::bytecode_size sz) {
    /*  Set bytecode size, so the viua::kernel::Kernel can stop execution even
     * if it doesn't reach HALT instruction but reaches bytecode address out of
     * bounds.
     */
    bytecode_size = sz;
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::mapfunction(
    const string& name,
    viua::internals::types::bytecode_size address) {
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::mapblock(
    const string& name,
    viua::internals::types::bytecode_size address) {
    /** Maps block name to bytecode address.
     */
    block_addresses[name] = address;
    return (*this);
}

viua::kernel::Kernel& viua::kernel::Kernel::register_external_function(
    const string& name,
    ForeignFunction* function_ptr) {
    /** Registers external function in viua::kernel::Kernel.
     */
    unique_lock<mutex> lock(foreign_functions_mutex);
    foreign_functions[name] = function_ptr;
    return (*this);
}


static auto is_native_module(string module) -> bool {
    auto double_colon = regex{"::"};
    auto oss          = ostringstream{};
    oss << regex_replace(module, double_colon, "/");

    auto const try_path = oss.str();
    auto path           = support::env::viua::get_mod_path(
        try_path, "vlib", support::env::get_paths("VIUAPATH"));
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(try_path, "vlib", VIUAPATH);
    }
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(
            try_path, "vlib", support::env::get_paths("VIUAAFTERPATH"));
    }
    return (path.size() > 0);
}
static auto is_foreign_module(string module) -> bool {
    auto path = support::env::viua::get_mod_path(
        module, "so", support::env::get_paths("VIUAPATH"));
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(module, "so", VIUAPATH);
    }
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(
            module, "so", support::env::get_paths("VIUAAFTERPATH"));
    }
    return (path.size() > 0);
}
void viua::kernel::Kernel::load_module(string module) {
    if (is_native_module(module)) {
        load_native_library(module);
    } else if (is_foreign_module(module)) {
        load_foreign_library(module);
    } else {
        throw make_unique<viua::types::Exception>(
            "LinkException", ("failed to link library: " + module));
    }
}
void viua::kernel::Kernel::load_native_library(const string& module) {
    regex double_colon("::");
    ostringstream oss;
    oss << regex_replace(module, double_colon, "/");
    string try_path = oss.str();
    auto path       = support::env::viua::get_mod_path(
        try_path, "vlib", support::env::get_paths("VIUAPATH"));
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(try_path, "vlib", VIUAPATH);
    }
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(
            try_path, "vlib", support::env::get_paths("VIUAAFTERPATH"));
    }

    if (path.size()) {
        Loader loader(path);
        loader.load();

        unique_ptr<viua::internals::types::byte[]> lnk_btcd{
            loader.get_bytecode()};

        auto const fn_names = loader.get_functions();
        auto const fn_addrs = loader.get_function_addresses();
        for (const auto& fn_linkname : fn_names) {
            linked_functions[fn_linkname] =
                pair<string, viua::internals::types::byte*>(
                    module, (lnk_btcd.get() + fn_addrs.at(fn_linkname)));
        }

        auto const bl_names = loader.get_blocks();
        auto const bl_addrs = loader.get_block_addresses();
        for (auto const& bl_linkname : bl_names) {
            linked_blocks[bl_linkname] =
                pair<string, viua::internals::types::byte*>(
                    module, (lnk_btcd.get() + bl_addrs.at(bl_linkname)));
        }

        linked_modules[module] =
            pair<viua::internals::types::bytecode_size,
                 unique_ptr<viua::internals::types::byte[]>>(
                loader.get_bytecode_size(), std::move(lnk_btcd));
    } else {
        throw make_unique<viua::types::Exception>("failed to link: " + module);
    }
}
void viua::kernel::Kernel::load_foreign_library(const string& module) {
    auto path = support::env::viua::get_mod_path(
        module, "so", support::env::get_paths("VIUAPATH"));
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(module, "so", VIUAPATH);
    }
    if (path.size() == 0) {
        path = support::env::viua::get_mod_path(
            module, "so", support::env::get_paths("VIUAAFTERPATH"));
    }

    if (path.size() == 0) {
        throw make_unique<viua::types::Exception>(
            "LinkException", ("failed to link library: " + module));
    }

    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if (handle == nullptr) {
        throw make_unique<viua::types::Exception>(
            "LinkException",
            ("failed to open handle: " + module + ": " + dlerror()));
    }

    using ExporterFunction   = const ForeignFunctionSpec* (*)();
    ExporterFunction exports = nullptr;
    if ((exports = reinterpret_cast<ExporterFunction>(dlsym(handle, "exports")))
        == nullptr) {
        throw make_unique<viua::types::Exception>(
            "failed to extract interface from module: " + module);
    }

    const ForeignFunctionSpec* exported = (*exports)();

    size_t i = 0;
    while (exported[i].name != nullptr) {
        register_external_function(exported[i].name, exported[i].fpointer);
        ++i;
    }

    cxx_dynamic_lib_handles.push_back(handle);
}

bool viua::kernel::Kernel::is_local_function(const string& name) const {
    return function_addresses.count(name);
}

bool viua::kernel::Kernel::is_linked_function(const string& name) const {
    return linked_functions.count(name);
}

bool viua::kernel::Kernel::is_native_function(const string& name) const {
    return (function_addresses.count(name) or linked_functions.count(name));
}

bool viua::kernel::Kernel::is_foreign_function(const string& name) const {
    return foreign_functions.count(name);
}

bool viua::kernel::Kernel::is_block(const string& name) const {
    return (block_addresses.count(name) or linked_blocks.count(name));
}

bool viua::kernel::Kernel::is_local_block(const string& name) const {
    return block_addresses.count(name);
}

bool viua::kernel::Kernel::is_linked_block(const string& name) const {
    return linked_blocks.count(name);
}

pair<viua::internals::types::byte*, viua::internals::types::byte*> viua::
    kernel::Kernel::get_entry_point_of_block(const std::string& name) const {
    viua::internals::types::byte* entry_point = nullptr;
    viua::internals::types::byte* module_base = nullptr;
    if (block_addresses.count(name)) {
        entry_point = (bytecode.get() + block_addresses.at(name));
        module_base = bytecode.get();
    } else {
        auto lf     = linked_blocks.at(name);
        entry_point = lf.second;
        module_base = linked_modules.at(lf.first).second.get();
    }
    return pair<viua::internals::types::byte*, viua::internals::types::byte*>(
        entry_point, module_base);
}

pair<viua::internals::types::byte*, viua::internals::types::byte*> viua::
    kernel::Kernel::get_entry_point_of(const std::string& name) const {
    viua::internals::types::byte* entry_point = nullptr;
    viua::internals::types::byte* module_base = nullptr;
    if (function_addresses.count(name)) {
        entry_point = (bytecode.get() + function_addresses.at(name));
        module_base = bytecode.get();
    } else {
        auto lf     = linked_functions.at(name);
        entry_point = lf.second;
        module_base = linked_modules.at(lf.first).second.get();
    }
    return pair<viua::internals::types::byte*, viua::internals::types::byte*>(
        entry_point, module_base);
}

void viua::kernel::Kernel::request_foreign_function_call(
    Frame* frame,
    viua::process::Process* requesting_process) {
    unique_lock<mutex> lock(foreign_call_queue_mutex);
    foreign_call_queue.emplace_back(
        make_unique<viua::scheduler::ffi::ForeignFunctionCallRequest>(
            frame, requesting_process, this));

    // unlock before calling notify_one() to avoid waking the worker thread when
    // it cannot obtain the lock and fetch the call request
    lock.unlock();
    foreign_call_queue_condition.notify_one();
}

void viua::kernel::Kernel::post_free_process(
    unique_ptr<viua::process::Process> p) {
    unique_lock<mutex> lock(free_virtual_processes_mutex);
    free_virtual_processes.emplace_back(std::move(p));
    lock.unlock();
    free_virtual_processes_cv.notify_one();
}

auto viua::kernel::Kernel::create_mailbox(const viua::process::PID pid)
    -> viua::internals::types::processes_count {
    unique_lock<mutex> lck(mailbox_mutex);
#if VIUA_VM_DEBUG_LOG
    cerr << "[kernel:mailbox:create] pid = " << pid.get() << endl;
#endif
    mailboxes.emplace(pid, Mailbox{});
    return ++running_processes;
}
auto viua::kernel::Kernel::delete_mailbox(const viua::process::PID pid)
    -> viua::internals::types::processes_count {
    unique_lock<mutex> lck(mailbox_mutex);
#if VIUA_VM_DEBUG_LOG
    cerr << "[kernel:mailbox:delete] pid = " << pid.get()
         << ", queued messages = " << mailboxes[pid].size() << endl;
#endif
    mailboxes.erase(pid);
    return --running_processes;
}
auto viua::kernel::Kernel::create_result_slot_for(viua::process::PID pid)
    -> void {
    unique_lock<mutex> lck{process_results_mutex};
    process_results.emplace(pid, ProcessResult{});
}
auto viua::kernel::Kernel::detach_process(const viua::process::PID pid)
    -> void {
    unique_lock<mutex> lck{process_results_mutex};
    process_results.erase(pid);
}
auto viua::kernel::Kernel::record_process_result(
    viua::process::Process* done_process) -> void {
    unique_lock<mutex> lck{process_results_mutex};

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
    const viua::process::PID pid) const -> bool {
    unique_lock<mutex> lck{process_results_mutex};
    return process_results.count(pid);
}
auto viua::kernel::Kernel::is_process_stopped(
    const viua::process::PID pid) const -> bool {
    unique_lock<mutex> lck{process_results_mutex};
    return process_results.at(pid).stopped();
}
auto viua::kernel::Kernel::is_process_terminated(
    const viua::process::PID pid) const -> bool {
    unique_lock<mutex> lck{process_results_mutex};
    return process_results.at(pid).terminated();
}
auto viua::kernel::Kernel::transfer_exception_of(const viua::process::PID pid)
    -> unique_ptr<viua::types::Value> {
    unique_lock<mutex> lck{process_results_mutex};
    auto tmp = process_results.at(pid).transfer_exception();
    process_results.erase(pid);
    return tmp;
}
auto viua::kernel::Kernel::transfer_result_of(const viua::process::PID pid)
    -> unique_ptr<viua::types::Value> {
    unique_lock<mutex> lck{process_results_mutex};
    auto tmp = process_results.at(pid).transfer_result();
    process_results.erase(pid);
    return tmp;
}

void viua::kernel::Kernel::send(const viua::process::PID pid,
                                unique_ptr<viua::types::Value> message) {
    unique_lock<mutex> lck(mailbox_mutex);
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
    queue<unique_ptr<viua::types::Value>>& message_queue) {
    unique_lock<mutex> lck(mailbox_mutex);
    if (mailboxes.count(pid) == 0) {
        throw make_unique<viua::types::Exception>("invalid PID");
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

uint64_t viua::kernel::Kernel::pids() const {
    return running_processes;
}

int viua::kernel::Kernel::exit() const {
    return return_code;
}

static auto no_of_schedulers(
    const char* env_name,
    viua::internals::types::schedulers_count default_limit)
    -> viua::internals::types::schedulers_count {
    decltype(default_limit) limit = default_limit;
    char* env_limit               = getenv(env_name);
    if (env_limit != nullptr) {
        int raw_limit = stoi(env_limit);
        if (raw_limit > 0) {
            limit = static_cast<decltype(limit)>(raw_limit);
        }
    }
    return limit;
}
auto viua::kernel::Kernel::no_of_vp_schedulers()
    -> viua::internals::types::schedulers_count {
    return no_of_schedulers("VIUA_VP_SCHEDULERS", default_vp_schedulers_limit);
}
auto viua::kernel::Kernel::no_of_ffi_schedulers()
    -> viua::internals::types::schedulers_count {
    return no_of_schedulers("VIUA_FFI_SCHEDULERS",
                            default_ffi_schedulers_limit);
}
auto viua::kernel::Kernel::is_tracing_enabled() -> bool {
    string viua_enable_tracing;
    char* env_text = getenv("VIUA_ENABLE_TRACING");
    if (env_text) {
        viua_enable_tracing = string(env_text);
    }
    return (viua_enable_tracing == "yes" or viua_enable_tracing == "true"
            or viua_enable_tracing == "1");
}

int viua::kernel::Kernel::run() {
    /*  VM viua::kernel::Kernel implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    vp_schedulers_limit = no_of_vp_schedulers();
    bool enable_tracing = is_tracing_enabled();

    vector<viua::scheduler::VirtualProcessScheduler> vp_schedulers;

    // reserver memory for all schedulers ahead of time
    vp_schedulers.reserve(vp_schedulers_limit);

    vp_schedulers.emplace_back(this,
                               &free_virtual_processes,
                               &free_virtual_processes_mutex,
                               &free_virtual_processes_cv,
                               enable_tracing);
    vp_schedulers.front().bootstrap(commandline_arguments);

    for (auto i = (vp_schedulers_limit - 1); i; --i) {
        vp_schedulers.emplace_back(this,
                                   &free_virtual_processes,
                                   &free_virtual_processes_mutex,
                                   &free_virtual_processes_cv);
    }

    for (auto& sched : vp_schedulers) {
        sched.launch();
    }

    for (auto& sched : vp_schedulers) {
        sched.shutdown();
        sched.join();
    }

    return_code = vp_schedulers.front().exit();

    return return_code;
}

viua::kernel::Kernel::Kernel()
        : bytecode(nullptr)
        , bytecode_size(0)
        , executable_offset(0)
        , return_code(0)
        , vp_schedulers_limit(default_vp_schedulers_limit)
        , ffi_schedulers_limit(default_ffi_schedulers_limit)
        , debug(false)
        , errors(false) {
    ffi_schedulers_limit = no_of_ffi_schedulers();
    for (auto i = ffi_schedulers_limit; i; --i) {
        foreign_call_workers.emplace_back(
            make_unique<std::thread>(viua::scheduler::ffi::ff_call_processor,
                                     &foreign_call_queue,
                                     &foreign_functions,
                                     &foreign_functions_mutex,
                                     &foreign_call_queue_mutex,
                                     &foreign_call_queue_condition));
    }
}

viua::kernel::Kernel::~Kernel() {
    /** Send a poison pill to every foreign function call worker thread.
     *  Collect them after they are killed.
     *
     * Use std::defer_lock to preven constructor from acquiring the mutex
     * .lock() is called manually later
     */
    std::unique_lock<std::mutex> lck(foreign_call_queue_mutex, std::defer_lock);
    for (auto i = foreign_call_workers.size(); i; --i) {
        // acquire the mutex for foreign call request queue
        lck.lock();

        // send poison pill;
        // one per worker thread since we can be sure that a thread consumes at
        // most one pill
        foreign_call_queue.push_back(nullptr);

        // release the mutex and notify worker thread that there is work to do
        // the thread consumes the pill and aborts
        lck.unlock();
        foreign_call_queue_condition.notify_all();
    }
    while (not foreign_call_workers.empty()) {
        // fetch handle for worker thread and
        // remove it from the list of workers
        auto w = std::move(foreign_call_workers.back());
        foreign_call_workers.pop_back();

        // join worker back to main thread and
        // delete it
        // by now, all workers should be killed by poison pills we sent them
        // earlier
        w->join();
    }

    for (auto const each : cxx_dynamic_lib_handles) {
        dlclose(each);
    }
}
