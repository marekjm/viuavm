/*
 *  Copyright (C) 2019 Marek Marecki
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

#include <iomanip>
#include <mutex>
#include <viua/kernel/kernel.h>
#include <viua/machine.h>
#include <viua/printutils.h>
#include <viua/process.h>
#include <viua/scheduler/process.h>
#include <viua/support/env.h>
#include <viua/types/exception.h>
#include <viua/types/function.h>
#include <viua/types/io.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
#include <viua/types/struct.h>
#include <viua/types/vector.h>

static auto print_stack_trace_default(viua::process::Process& process) -> void {
    auto const trace = process.trace();
    std::cerr << "stack trace: from entry point, most recent call last...\n";
    auto i = decltype(trace)::size_type{0};
    if (viua::support::env::get_var("VIUA_STACK_TRACES") != "full") {
        i = (trace.size() and trace[0]->function_name == "__entry");
    }
    for (; i < trace.size(); ++i) {
        std::cerr << "  " << stringify_function_invocation(trace[i]) << "\n";
    }
    std::cerr << "\n";

    auto const thrown_object = std::move(process.transfer_active_exception());
    auto const ex = dynamic_cast<viua::types::Exception*>(thrown_object.get());
    auto const ex_type = thrown_object->type();

    // cerr << "failed instruction: " <<
    // get<0>(disassembler::instruction(process->execution_at())) << endl;
    std::cerr << "uncaught object: " << ex_type << " = "
              << (ex ? ex->what() : thrown_object->str()) << "\n";
    std::cerr << "\n";

    std::cerr << "frame details:\n";

    if (trace.size()) {
        auto const last = trace.back();
        if (last->local_register_set.owns()
            and last->local_register_set->size()) {
            auto non_empty = unsigned{0};
            for (decltype(last->local_register_set->size()) r = 0;
                 r < last->local_register_set->size();
                 ++r) {
                if (last->local_register_set->at(r) != nullptr) {
                    ++non_empty;
                }
            }
            std::cerr << "  non-empty registers: " << non_empty << '/'
                      << last->local_register_set->size();
            std::cerr << (non_empty ? ":\n" : "\n");
            for (decltype(last->local_register_set->size()) r = 0;
                 r < last->local_register_set->size();
                 ++r) {
                if (last->local_register_set->at(r) == nullptr) {
                    continue;
                }
                std::cerr << "    registers[" << r << "]: ";
                std::cerr << '<' << last->local_register_set->get(r)->type()
                          << "> " << last->local_register_set->get(r)->str()
                          << "\n";
            }
        } else if (not last->local_register_set.owns()) {
            std::cerr << "  this frame did not own its registers\n";
        } else {
            std::cerr << "  no registers were allocated for this frame\n";
        }

        if (last->arguments->size()) {
            std::cerr << "  non-empty arguments (out of "
                      << last->arguments->size() << "):\n";
            for (decltype(last->arguments->size()) r = 0;
                 r < last->arguments->size();
                 ++r) {
                if (last->arguments->at(r) == nullptr) {
                    continue;
                }
                std::cerr << "    arguments[" << r << "]: ";
                if (last->arguments->isflagged(r, MOVED)) {
                    std::cerr << "[moved] ";
                }
                if (auto ptr = dynamic_cast<viua::types::Pointer*>(
                        last->arguments->get(r))) {
                    if (ptr->expired()) {
                        std::cerr << "<ExpiredPointer>"
                                  << "\n";
                    } else {
                        std::cerr << '<' << ptr->type() << ">\n";
                    }
                } else {
                    std::cerr << '<' << last->arguments->get(r)->type() << "> "
                              << last->arguments->get(r)->str() << "\n";
                }
            }
        } else {
            std::cerr << "  no arguments were passed to this frame\n";
        }
    } else {
        std::cerr << "no stack trace available\n";
    }
}
static auto print_stack_trace_json(viua::process::Process& process) -> void {
    std::ostringstream oss;
    oss << '{';

    oss << "\"trace\":[";
    auto trace = process.trace();
    auto i     = decltype(trace)::size_type{0};
    if (viua::support::env::get_var("VIUA_STACK_TRACES") != "full") {
        i = (trace.size() and trace[0]->function_name == "__entry");
    }
    for (; i < trace.size(); ++i) {
        oss << str::enquote(stringify_function_invocation(trace[i]));
        if (i < trace.size() - 1) {
            oss << ',';
        }
    }
    oss << "],";

    std::unique_ptr<viua::types::Value> thrown_object(
        process.transfer_active_exception());
    oss << "\"uncaught\":{";
    oss << "\"type\":" << str::enquote(thrown_object->type()) << ',';
    oss << "\"value\":" << str::enquote(thrown_object->str());
    oss << "},";

    oss << "\"frame\":{";
    oss << "}";

    oss << '}';

    auto to = viua::support::env::get_var("VIUA_STACKTRACE_PRINT_TO");
    if (to == "") {
        to = "stdout";
    }
    if (to == "stdout") {
        std::cout << oss.str() << "\n";
    } else if (to == "stderr") {
        std::cerr << oss.str() << "\n";
    } else {
        std::cerr << oss.str() << "\n";
    }
}
static void print_stack_trace(viua::process::Process& process) {
    auto stack_trace_print_type =
        viua::support::env::get_var("VIUA_STACKTRACE_SERIALISATION");
    if (stack_trace_print_type == "") {
        stack_trace_print_type = "default";
    }

    if (stack_trace_print_type == "default") {
        print_stack_trace_default(process);
    } else if (stack_trace_print_type == "json") {
        print_stack_trace_json(process);
    } else {
        print_stack_trace_default(process);
    }
}

namespace viua { namespace scheduler {
auto Process_scheduler::push(std::unique_ptr<process_type> proc) -> void {
    std::lock_guard<std::mutex> lck{process_queue_mtx};
    process_queue.push_back(std::move(proc));
}
auto Process_scheduler::pop() -> std::unique_ptr<process_type> {
    std::lock_guard<std::mutex> lck{process_queue_mtx};
    auto proc = std::move(process_queue.front());
    process_queue.pop_front();
    return proc;
}
auto Process_scheduler::size() const -> size_type {
    std::lock_guard<std::mutex> lck{process_queue_mtx};
    return process_queue.size();
}
auto Process_scheduler::empty() const -> bool {
    std::lock_guard<std::mutex> lck{process_queue_mtx};
    return process_queue.empty();
}

Process_scheduler::Process_scheduler(viua::kernel::Kernel& k, id_type const x)
        : assigned_id{x}, attached_kernel{k} {}

Process_scheduler::~Process_scheduler() {}

auto Process_scheduler::id() const -> id_type {
    return assigned_id;
}

auto Process_scheduler::bootstrap(std::vector<std::string> args) -> void {
    auto initial_frame           = std::make_unique<Frame>(nullptr, 0);
    initial_frame->function_name = ENTRY_FUNCTION_NAME;
    initial_frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(2));

    auto cmdline = std::make_unique<viua::types::Vector>();
    for (auto const& each : args) {
        cmdline->push(std::make_unique<viua::types::String>(each));
    }
    initial_frame->local_register_set->set(1, std::move(cmdline));

    main_process = spawn(std::move(initial_frame), nullptr, true);
    main_process->priority(16);
    main_process->pin();
}

auto Process_scheduler::spawn(std::unique_ptr<Frame> frame,
                              process_type* parent,
                              bool disown) -> viua::process::Process* {
    auto process = std::make_unique<process_type>(
        std::move(frame), this, parent, false /* tracing_enabled */
    );

    process->start();
    if (disown) {
        process->detach();
    }

    auto pid_of_new_process = process->pid();
    attached_kernel.create_mailbox(pid_of_new_process);
    if (not disown) {
        attached_kernel.create_result_slot_for(pid_of_new_process);
    }

    // FIXME This is fishy, and probably unsafe. Obtaining a pointer to this
    // newly spawned process, pushing it onto the process queue, and then using
    // the saved pointer for things... sounds like asking for problems.
    auto process_ptr = process.get();

    push(std::move(process));

    attached_kernel.notify_about_process_spawned(this);

    return process_ptr;
}
auto Process_scheduler::give_up_processes()
    -> std::vector<std::unique_ptr<process_type>> {
    std::lock_guard<std::mutex> lck{process_queue_mtx};

    /*
     * If we have no or only one process then do not give up anything. It would
     * be counterproductive to give up our only process, be left with no work to
     * do, and go straight into the wait queue to steal some processes...
     */
    if (process_queue.empty() or (process_queue.size() == 1)) {
        return {};
    }

    auto const give_up_limit =
        static_cast<int64_t>(((process_queue.size() * 100) / 141)
                             - process_queue.size())
        * -1;

    auto given_up = std::vector<std::unique_ptr<process_type>>{};
    auto saved    = std::vector<std::unique_ptr<process_type>>{};

    for (auto i = 0; i < give_up_limit; ++i) {
        auto proc = std::move(process_queue.front());
        process_queue.pop_front();
        if (proc->pinned()) {
            saved.push_back(std::move(proc));
        } else {
            given_up.push_back(std::move(proc));
        }
    }

    for (auto& proc : saved) {
        process_queue.push_back(std::move(proc));
    }

    return given_up;
}

auto Process_scheduler::is_joinable(viua::process::PID const pid) const
    -> bool {
    return attached_kernel.is_process_joinable(pid);
}
auto Process_scheduler::is_stopped(viua::process::PID const pid) const -> bool {
    return attached_kernel.is_process_stopped(pid);
}
auto Process_scheduler::is_terminated(viua::process::PID const pid) const
    -> bool {
    return attached_kernel.is_process_terminated(pid);
}

auto Process_scheduler::transfer_exception_of(
    viua::process::PID const pid) const -> std::unique_ptr<viua::types::Value> {
    return attached_kernel.transfer_exception_of(pid);
}
auto Process_scheduler::transfer_result_of(viua::process::PID const pid) const
    -> std::unique_ptr<viua::types::Value> {
    return attached_kernel.transfer_result_of(pid);
}

auto Process_scheduler::send(viua::process::PID const pid,
                             std::unique_ptr<viua::types::Value> message)
    -> void {
    attached_kernel.send(pid, std::move(message));
}
auto Process_scheduler::receive(
    viua::process::PID const pid,
    std::queue<std::unique_ptr<viua::types::Value>>& message_queue) -> void {
    attached_kernel.receive(pid, message_queue);
}

auto Process_scheduler::request_ffi_call(std::unique_ptr<Frame> frame,
                                         viua::process::Process& p) -> void {
    attached_kernel.request_foreign_function_call(std::move(frame), p);
}

auto Process_scheduler::is_native_function(std::string const name) const
    -> bool {
    return attached_kernel.is_native_function(name);
}
auto Process_scheduler::is_foreign_function(std::string const name) const
    -> bool {
    return attached_kernel.is_foreign_function(name);
}
auto Process_scheduler::is_block(std::string const name) const -> bool {
    return attached_kernel.is_block(name);
}

auto Process_scheduler::load_module(std::string const name) -> void {
    attached_kernel.load_module(name);
}


auto Process_scheduler::get_entry_point_of_block(std::string const name) const
    -> std::pair<viua::internals::types::Op_address_type,
                 viua::internals::types::Op_address_type> {
    return attached_kernel.get_entry_point_of_block(name);
}
auto Process_scheduler::get_entry_point_of_function(std::string const& name)
    const -> std::pair<viua::internals::types::Op_address_type,
                       viua::internals::types::Op_address_type> {
    return attached_kernel.get_entry_point_of(name);
}

template<typename T> struct deferred {
    T const& fn_to_call;

    deferred(T const& fn) : fn_to_call{fn} {}
    deferred(deferred<T> const&) = delete;
    auto operator=(deferred<T> const&) = delete;
    deferred(deferred<T>&&)            = delete;
    auto operator=(deferred<T>&&) = delete;
    inline ~deferred() {
        fn_to_call();
    }
};

auto Process_scheduler::launch() -> void {
    scheduler_thread = std::thread([this] { (*this)(); });
}
auto Process_scheduler::operator()() -> void {
    /*
     * Used to check if any processes "ticked". A ticking process either did
     * some work, or is in a suspended state awaiting a wake-up signal (so has
     * potential to do useful work in the future).
     */
    auto ticked = false;

    /*
     * This was used by the old scheduler to enter a short waiting period when
     * it detected that there were no active processes. It would then wait for
     * any process to become active (e.g. after returning from an FFI call or
     * from a timeout).
     */
    auto any_active = false;

    while (true) {
        if (empty()) {
            auto stolen_processes = attached_kernel.steal_processes();
            if (stolen_processes.empty()) {
                /*
                 *  If there are no processes to steal, there is nothing we can
                 * do but shut down.
                 */
                if (attached_kernel.process_count() == 0) {
                    break;
                }

                /*
                 *  If there are processes running on the VM, but we were not
                 *  able to steal any of them, let's wait again and see if the
                 *  situation will change.
                 */
                continue;
            }

            {
                /*
                 * Obtain the lock once for all of the stolen processes instead
                 * of using the push() function which would pay the cost of
                 * acquiring the lock for every process pushed.
                 */
                std::lock_guard<std::mutex> lck{process_queue_mtx};
                for (auto& each : stolen_processes) {
                    process_queue.push_back(std::move(each));
                }
            }

            continue;
        }

        auto a_process = pop();

        if (a_process->stopped() and a_process->joinable()) {
            // return false in execute_quant()
            push(std::move(a_process));
            continue;
        }

#if 0
        auto const push_the_process_back = deferred([&a_process, this]{
            process_queue.push_back(std::move(a_process));
        });
#endif

        if (a_process->suspended()) {
            // return true in execute_quant()
            push(std::move(a_process));
            continue;
        }

        constexpr auto CYCLES_PER_BURST = uint32_t{256};
        for (auto i = CYCLES_PER_BURST; i; --i) {
            if (a_process->stopped()) {
                /*
                 * Remember to break if the process stopped
                 * otherwise the kernel will try to tick the process and
                 * it will crash (will try to execute instructions from 0x0
                 * pointer).
                 */
                break;
            }
            if (a_process->suspended()) {
                /*
                 * Do not execute suspended processes.
                 */
                break;
            }

            a_process->tick();
        }

        any_active =
            (any_active
             or ((not a_process->stopped()) and (not a_process->suspended())));
        ticked =
            (ticked or (not a_process->stopped()) or a_process->suspended());

        if (a_process->suspended()) {
            /*
             * This check is required to avoid race condition later in the
             * function. When a process is suspended its state cannot really be
             * detected correctly except for the fact that the process is still
             * possibly running.
             *
             * Race condition arises when an exception is thrown during an FFI
             * call; as FFI results and exceptions are transferred back
             * asynchronously a following sequence of events may occur:
             *
             * - process requests FFI call, and is suspended
             * - an exception is thrown during FFI call
             * - the exception is registered in a process, and the process
             *   enters "terminated" state
             * - the process is woken up, and enters "stopped" state
             *
             * Now, if the process is woken up between the next if (the "if
             * terminated") and the "if stopped" one it will be marked as dead
             * without handling the exception because when the process scheduler
             * was checking for presence of an exception it was not there yet.
             *
             * Checking if the process is suspended here and, if it is,
             * immediately marking it as running prevents the race condition.
             *
             * REMEMBER: the last thing that is done after servicing an FFI call
             * is waking the process up so as long as the process is suspended
             * it must be considered to be running.
             */
            push(std::move(a_process));
            continue;
        }
        if (a_process->terminated() and not a_process->joinable()
            and a_process->parent() == nullptr) {
            if (not a_process->watchdogged()) {
                if (a_process.get() == main_process
                    and not exit_code.has_value()) {
                    exit_code = 1;
                }
                print_stack_trace(*a_process.get());
                attached_kernel.delete_mailbox(a_process->pid());
                attached_kernel.notify_about_process_death();
                continue;
            } else {
                if (a_process->trace().at(0)->function_name
                    == a_process->watchdog()) {
                    /*
                     * If the topmost function is the watchdog it means that the
                     * watchdog has crashed. We have no choice but to reap the
                     * process as it failed to save itself.
                     */
                    std::cerr << "[scheduler][id=" << std::hex << std::setw(4)
                              << std::setfill('0') << id() << std::dec
                              << "][pid=" << a_process->pid().get()
                              << "] watchdog failed, the process is broken "
                                 "beyond repair\n";

                    print_stack_trace(*a_process.get());
                    attached_kernel.delete_mailbox(a_process->pid());
                    attached_kernel.notify_about_process_death();
                    if (a_process.get() == main_process
                        and not exit_code.has_value()) {
                        exit_code = 1;
                    }
                    continue;
                }

                std::cerr << "[scheduler][id=" << std::hex << std::setw(4)
                          << std::setfill('0') << id() << std::dec
                          << "][pid=" << a_process->pid().get()
                          << "] the process has crashed, calling watchdog: "
                          << a_process->watchdog() << "\n";

                auto parameters = std::make_unique<viua::types::Vector>();
                auto top_args   = a_process->trace().at(0)->arguments.get();
                for (auto j = decltype(top_args->size()){0};
                     j < top_args->size();
                     ++j) {
                    if (top_args->at(j)) {
                        parameters->push(top_args->pop(j));
                    }
                }

                auto death_message = std::make_unique<viua::types::Struct>();
                death_message->insert(
                    "function",
                    std::make_unique<viua::types::Function>(
                        a_process->trace().at(0)->function_name));
                auto exc = a_process->transfer_active_exception();
                death_message->insert("exception", std::move(exc));
                death_message->insert("parameters", std::move(parameters));

                auto death_frame = std::move(a_process->frame_for_watchdog());
                death_frame->arguments->set(0, std::move(death_message));
                a_process->become(a_process->watchdog(),
                                  std::move(death_frame));
                push(std::move(a_process));
                ticked = true;
                continue;
            }
        }

        if (a_process->stopped()) {
            attached_kernel.record_process_result(a_process.get());
            attached_kernel.delete_mailbox(a_process->pid());
            attached_kernel.notify_about_process_death();
            if (a_process.get() == main_process and not exit_code.has_value()) {
                auto const ret = a_process->get_return_value();
                auto const ret_int =
                    static_cast<viua::types::Integer*>(ret.get());
                exit_code = (ret ? static_cast<int>(ret_int->as_integer()) : 0);
            }
        } else {
            push(std::move(a_process));
        }

        if (not ticked) {
            /*
             * FIXME Instead of breaking we should perform rebalancing. Steal
             * some running processes from other schedulers.
             */
            auto const n = attached_kernel.process_count();
            if (n == 0) {
                break;
            }
        }
    }
}

auto Process_scheduler::shutdown() -> void {}
auto Process_scheduler::join() -> void {
    scheduler_thread.join();
}
auto Process_scheduler::exit() const -> int {
    return exit_code.value_or(0);
}

auto Process_scheduler::schedule_io(
    std::unique_ptr<viua::scheduler::io::IO_interaction> i) -> void {
    attached_kernel.schedule_io(std::move(i));
}

auto Process_scheduler::cancel_io(
    viua::scheduler::io::IO_interaction::id_type const interaction_id) -> void {
    attached_kernel.cancel_io(interaction_id);
}

auto Process_scheduler::io_complete(
    viua::scheduler::io::IO_interaction::id_type const interaction_id) const
    -> bool {
    return attached_kernel.io_complete(interaction_id);
}

auto Process_scheduler::io_result(
    viua::scheduler::io::IO_interaction::id_type const interaction_id)
    -> std::unique_ptr<viua::types::Value> {
    return attached_kernel.io_result(interaction_id);
}
}}  // namespace viua::scheduler
