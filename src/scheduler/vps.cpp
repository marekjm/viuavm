/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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

#include <sstream>
#include <string>
#include <vector>
#include <viua/kernel/kernel.h>
#include <viua/machine.h>
#include <viua/printutils.h>
#include <viua/process.h>
#include <viua/scheduler/vps.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/types/struct.h>
#include <viua/types/exception.h>
#include <viua/types/function.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
using namespace std;


static auto print_stack_trace_default(viua::process::Process* process) -> void {
    auto trace = process->trace();
    cerr << "stack trace: from entry point, most recent call last...\n";
    decltype(trace)::size_type i = 0;
    if (support::env::get_var("VIUA_STACK_TRACES") != "full") {
        i = (trace.size() and trace[0]->function_name == "__entry");
    }
    for (; i < trace.size(); ++i) {
        cerr << "  " << stringify_function_invocation(trace[i]) << "\n";
    }
    cerr << "\n";

    unique_ptr<viua::types::Value> thrown_object(
        process->transfer_active_exception());
    auto ex        = dynamic_cast<viua::types::Exception*>(thrown_object.get());
    string ex_type = thrown_object->type();

    // cerr << "failed instruction: " <<
    // get<0>(disassembler::instruction(process->execution_at())) << endl;
    cerr << "uncaught object: " << ex_type << " = "
         << (ex ? ex->what() : thrown_object->str()) << endl;
    cerr << "\n";

    cerr << "frame details:\n";

    if (trace.size()) {
        Frame* last = trace.back();
        if (last->local_register_set.owns()
            and last->local_register_set->size()) {
            unsigned non_empty = 0;
            for (decltype(last->local_register_set->size()) r = 0;
                 r < last->local_register_set->size();
                 ++r) {
                if (last->local_register_set->at(r) != nullptr) {
                    ++non_empty;
                }
            }
            cerr << "  non-empty registers: " << non_empty << '/'
                 << last->local_register_set->size();
            cerr << (non_empty ? ":\n" : "\n");
            for (decltype(last->local_register_set->size()) r = 0;
                 r < last->local_register_set->size();
                 ++r) {
                if (last->local_register_set->at(r) == nullptr) {
                    continue;
                }
                cerr << "    registers[" << r << "]: ";
                cerr << '<' << last->local_register_set->get(r)->type() << "> "
                     << last->local_register_set->get(r)->str() << endl;
            }
        } else if (not last->local_register_set.owns()) {
            cerr << "  this frame did not own its registers" << endl;
        } else {
            cerr << "  no registers were allocated for this frame" << endl;
        }

        if (last->arguments->size()) {
            cerr << "  non-empty arguments (out of " << last->arguments->size()
                 << "):" << endl;
            for (decltype(last->arguments->size()) r = 0;
                 r < last->arguments->size();
                 ++r) {
                if (last->arguments->at(r) == nullptr) {
                    continue;
                }
                cerr << "    arguments[" << r << "]: ";
                if (last->arguments->isflagged(r, MOVED)) {
                    cerr << "[moved] ";
                }
                if (auto ptr = dynamic_cast<viua::types::Pointer*>(
                        last->arguments->get(r))) {
                    if (ptr->expired()) {
                        cerr << "<ExpiredPointer>" << endl;
                    } else {
                        cerr << '<' << ptr->type() << '>' << endl;
                    }
                } else {
                    cerr << '<' << last->arguments->get(r)->type() << "> "
                         << last->arguments->get(r)->str() << endl;
                }
            }
        } else {
            cerr << "  no arguments were passed to this frame" << endl;
        }
    } else {
        cerr << "no stack trace available" << endl;
    }
}
static auto print_stack_trace_json(viua::process::Process* process) -> void {
    ostringstream oss;
    oss << '{';

    oss << "\"trace\":[";
    auto trace                   = process->trace();
    decltype(trace)::size_type i = 0;
    if (support::env::get_var("VIUA_STACK_TRACES") != "full") {
        i = (trace.size() and trace[0]->function_name == "__entry");
    }
    for (; i < trace.size(); ++i) {
        oss << str::enquote(stringify_function_invocation(trace[i]));
        if (i < trace.size() - 1) {
            oss << ',';
        }
    }
    oss << "],";

    unique_ptr<viua::types::Value> thrown_object(
        process->transfer_active_exception());
    oss << "\"uncaught\":{";
    oss << "\"type\":" << str::enquote(thrown_object->type()) << ',';
    oss << "\"value\":" << str::enquote(thrown_object->str());
    oss << "},";

    oss << "\"frame\":{";
    oss << "}";

    oss << '}';

    auto to = support::env::get_var("VIUA_STACKTRACE_PRINT_TO");
    if (to == "") {
        to = "stdout";
    }
    if (to == "stdout") {
        cout << oss.str() << endl;
    } else if (to == "stderr") {
        cerr << oss.str() << endl;
    } else {
        cerr << oss.str() << endl;
    }
}
static void print_stack_trace(viua::process::Process* process) {
    auto stack_trace_print_type =
        support::env::get_var("VIUA_STACKTRACE_SERIALISATION");
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

template<class T> static void viua_err(ostringstream& oss, T&& arg) {
    oss << arg;
}
template<class T, class... Ts>
static void viua_err(ostringstream& oss, T&& arg, Ts&&... Rest) {
    oss << arg;
    viua_err(oss, std::forward<Ts>(Rest)...);
}

template<class... Ts> static void viua_err(Ts&&... arguments) {
    ostringstream oss;
    oss << std::chrono::steady_clock::now().time_since_epoch().count() << ' ';
    viua_err(oss, std::forward<Ts>(arguments)...);
    cerr << (oss.str() + '\n');
}


bool viua::scheduler::VirtualProcessScheduler::execute_quant(
    viua::process::Process* th,
    viua::internals::types::process_time_slice_type priority) {
    if (th->stopped() and th->joinable()) {
        // stopped but still joinable
        // we don't have to deal with "stopped and unjoinable" case here
        // because it is handled later (after ticking code)
        return false;
    }
    if (th->suspended()) {
        // do not execute suspended processes
        return true;
    }

    for (decltype(priority) j = 0; (priority == 0 or j < priority); ++j) {
        if (th->stopped()) {
            // remember to break if the process stopped
            // otherwise the kernel will try to tick the process and
            // it will crash (will try to execute instructions from 0x0 pointer)
            break;
        }
        if (th->suspended()) {
            // do not execute suspended processes
            break;
        }
#if VIUA_VM_DEBUG_LOG
        viua_err("[sched:vps:quant] pid = ", th->pid().get(), ", tick = ", j);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        th->tick();
    }

    return true;
}

viua::kernel::Kernel* viua::scheduler::VirtualProcessScheduler::kernel() const {
    return attached_kernel;
}

bool viua::scheduler::VirtualProcessScheduler::is_local_function(
    const string& name) const {
    return attached_kernel->is_local_function(name);
}

bool viua::scheduler::VirtualProcessScheduler::is_linked_function(
    const string& name) const {
    return attached_kernel->is_linked_function(name);
}

bool viua::scheduler::VirtualProcessScheduler::is_native_function(
    const string& name) const {
    return attached_kernel->is_native_function(name);
}

bool viua::scheduler::VirtualProcessScheduler::is_foreign_function(
    const string& name) const {
    return attached_kernel->is_foreign_function(name);
}

bool viua::scheduler::VirtualProcessScheduler::is_block(
    const string& name) const {
    return attached_kernel->is_block(name);
}

bool viua::scheduler::VirtualProcessScheduler::is_local_block(
    const string& name) const {
    return attached_kernel->is_local_block(name);
}

bool viua::scheduler::VirtualProcessScheduler::is_linked_block(
    const string& name) const {
    return attached_kernel->is_linked_block(name);
}

pair<viua::internals::types::byte*, viua::internals::types::byte*> viua::
    scheduler::VirtualProcessScheduler::get_entry_point_of_block(
        const std::string& name) const {
    return attached_kernel->get_entry_point_of_block(name);
}

pair<viua::internals::types::byte*, viua::internals::types::byte*> viua::
    scheduler::VirtualProcessScheduler::get_entry_point_of(
        const std::string& name) const {
    return attached_kernel->get_entry_point_of(name);
}

void viua::scheduler::VirtualProcessScheduler::request_foreign_function_call(
    Frame* frame,
    viua::process::Process* p) const {
    attached_kernel->request_foreign_function_call(frame, p);
}

void viua::scheduler::VirtualProcessScheduler::load_module(string module) {
    attached_kernel->load_module(module);
}

auto viua::scheduler::VirtualProcessScheduler::cpi() const
    -> decltype(processes)::size_type {
    return current_process_index;
}


auto viua::scheduler::VirtualProcessScheduler::size() const
    -> decltype(processes)::size_type {
    return processes.size();
}

viua::process::Process* viua::scheduler::VirtualProcessScheduler::process(
    decltype(processes)::size_type index) {
    return processes.at(index).get();
}

viua::process::Process* viua::scheduler::VirtualProcessScheduler::process() {
    return process(current_process_index);
}

viua::process::Process* viua::scheduler::VirtualProcessScheduler::spawn(
    unique_ptr<Frame> frame,
    viua::process::Process* parent,
    bool disown) {
    auto p = make_unique<viua::process::Process>(
        std::move(frame), this, parent, tracing_enabled);
    p->begin();
    if (disown) {
        p->detach();
    }

    viua::process::Process* process_ptr = p.get();
    const auto total_processes =
        attached_kernel->create_mailbox(process_ptr->pid());
    if (not disown) {
        attached_kernel->create_result_slot_for(process_ptr->pid());
    }
    const auto running_schedulers = attached_kernel->no_of_vp_schedulers();

    /*
     * Determine if this scheduler is overburdened and should post processes to
     * kernel.
     *
     * The algorithm is simple: if current load is slightly more than our fair
     * share, post spawned process to kernel and let other schedulers handle it.
     * The "slightly more" part is there because we try to be a good scheduler
     * and don't just push our processes to others as soon as we're a little bit
     * tired. Also, because the processes may be short-lived and the costs of
     * synchronisation may outweight the benefits of increased concurrency.
     *
     * Benchmarks proved that dealing locally with increased load for a period
     * of time before posting processes to kernel is a better solution than
     * posting immediately. This was measured using lots of short-lived
     * processes in the "N bottles of beer" bechmark: 16384 bottles, 8
     * schedulers, on a CPU with 4 physical cores.
     */
    if (processes.size()
        > ((total_processes / running_schedulers) / 100 * 140)) {
#if VIUA_VM_DEBUG_LOG
        viua_err("[scheduler:vps:",
                 this,
                 "] posting process ",
                 p.get(),
                 ":",
                 p->starting_function(),
                 " to kernel");
#endif
        attached_kernel->post_free_process(std::move(p));
    } else {
#if VIUA_VM_DEBUG_LOG
        viua_err("[scheduler:vps:",
                 this,
                 "] retaining process ",
                 p.get(),
                 ":",
                 p->starting_function());
#endif
        processes.emplace_back(std::move(p));
    }

    return process_ptr;
}

void viua::scheduler::VirtualProcessScheduler::send(
    const viua::process::PID pid,
    unique_ptr<viua::types::Value> message) {
#if VIUA_VM_DEBUG_LOG
    viua_err("[sched:vps:send] pid = ", pid.get());
#endif
    attached_kernel->send(pid, std::move(message));
}

void viua::scheduler::VirtualProcessScheduler::receive(
    const viua::process::PID pid,
    queue<unique_ptr<viua::types::Value>>& message_queue) {
#if VIUA_VM_DEBUG_LOG
    viua_err("[sched:vps:receive] pid = ", pid.get());
#endif
    attached_kernel->receive(pid, message_queue);
}

auto viua::scheduler::VirtualProcessScheduler::is_joinable(
    const viua::process::PID pid) const -> bool {
    return attached_kernel->is_process_joinable(pid);
}
auto viua::scheduler::VirtualProcessScheduler::is_stopped(
    const viua::process::PID pid) const -> bool {
    return attached_kernel->is_process_stopped(pid);
}
auto viua::scheduler::VirtualProcessScheduler::is_terminated(
    const viua::process::PID pid) const -> bool {
    return attached_kernel->is_process_terminated(pid);
}
auto viua::scheduler::VirtualProcessScheduler::transfer_exception_of(
    const viua::process::PID pid) const -> unique_ptr<viua::types::Value> {
    return attached_kernel->transfer_exception_of(pid);
}
auto viua::scheduler::VirtualProcessScheduler::transfer_result_of(
    const viua::process::PID pid) const -> unique_ptr<viua::types::Value> {
    return attached_kernel->transfer_result_of(pid);
}

bool viua::scheduler::VirtualProcessScheduler::burst() {
    if (not processes.size()) {
        // make kernel stop if there are no processes_list to run
        return false;
    }

    bool ticked     = false;
    bool any_active = false;

    vector<unique_ptr<viua::process::Process>> running_processes_list;
    decltype(running_processes_list) dead_processes_list;
    current_load = 0;
    for (decltype(running_processes_list)::size_type i = 0;
         i < processes.size();
         ++i) {
        current_process_index = i;
        auto th               = processes.at(i).get();

#if VIUA_VM_DEBUG_LOG
        viua_err("[sched:vps:burst] pid = ", th->pid().get());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        execute_quant(th, th->priority());
        any_active =
            (any_active or ((not th->stopped()) and (not th->suspended())));
        ticked = (ticked or (not th->stopped()) or th->suspended());

        if (not(th->suspended() or th->terminated() or th->stopped())) {
            ++current_load;
        }

        if (th->suspended()) {
            // This check is required to avoid race condition later in the
            // function. When a process is suspended its state cannot really be
            // detected correctly except for the fact that the process is still
            // possibly running.
            //
            // Race condition arises when an exception is thrown during an FFI
            // call; as FFI results and exceptions are transferred back
            // asynchronously a following sequence of events may occur:
            //
            //  - process requests FFI call, and is suspended
            //  - an exception is thrown during FFI call
            //  - the exception is registered in a process, and the process
            //  enters "terminated" state
            //  - the process is woken up, and enters "stopped" state
            //
            // Now, if the process is woken up between the next if (the `if
            // (th-terminated() ...)`), and the `if (th->stopped() ...)` one it
            // will be marked as dead without handling the exception because
            // when the VPS was checking for presence of an exception it was not
            // there yet.
            //
            // Checking if the process is suspended here and, if it is,
            // immediately marking it as running prevents the race condition.
            //
            // REMEMBER: the last thing that is done after servicing an FFI call
            // is waking the process up so as long as the process is suspended
            // it must be considered to be running.
            running_processes_list.emplace_back(std::move(processes.at(i)));
            continue;
        }

        if (th->terminated() and not th->joinable()
            and th->parent() == nullptr) {
#if VIUA_VM_DEBUG_LOG
            viua_err("[sched:vps:died] pid = ", th->pid().get());
#endif
            if (not th->watchdogged()) {
                if (th == main_process) {
                    exit_code = 1;
                }

                auto trace = th->trace();

                ostringstream errss;
#if VIUA_VM_DEBUG_LOG
                viua_err(errss,
                         "process ",
                         current_process_index,
                         " spawned using ");
#endif
                if (trace.size() > 1) {
// if trace size if greater than one, detect if this is main process
#if VIUA_VM_DEBUG_LOG
                    viua_err(
                        errss,
                        trace[(trace[0]->function_name == ENTRY_FUNCTION_NAME)]
                            ->function_name);
#endif
                } else if (trace.size() == 1) {
// if trace size is equal to one, just print the top-most function
#if VIUA_VM_DEBUG_LOG
                    viua_err(errss, trace[0]->function_name);
#endif
                } else {
// in all other cases print the function the process has been started with
// it is a safe bet (perhaps even safer than printing the top-most function on
// the stack as that may have been changed by a tail call...)
#if VIUA_VM_DEBUG_LOG
                    viua_err(errss, th->starting_function());
#endif
                }
#if VIUA_VM_DEBUG_LOG
                viua_err(errss, " has terminated");
#endif
                cerr << (errss.str() + '\n');

                print_stack_trace(th);

                attached_kernel->delete_mailbox(th->pid());
// push broken process to dead processes_list list to
// erase it later
#if VIUA_VM_DEBUG_LOG
                viua_err("[scheduler:vps] process ",
                         processes.at(i).get(),
                         ": marked as dead");
#endif
                dead_processes_list.emplace_back(std::move(processes.at(i)));
            } else {
                if (th->trace().at(0)->function_name == th->watchdog()) {
#if VIUA_VM_DEBUG_LOG
                    viua_err("[sched:vps:died:in-watchdog] pid = ",
                             th->pid().get(),
                             "; process reaped, death cause: ",
                             exc->str());
#endif

                    print_stack_trace(th);
                    dead_processes_list.emplace_back(std::move(processes.at(i)));
                    continue;
                }

                auto parameters = make_unique<viua::types::Vector>();
                viua::kernel::RegisterSet* top_args =
                    th->trace().at(0)->arguments.get();
                for (decltype(top_args->size()) j = 0; j < top_args->size();
                     ++j) {
                    if (top_args->at(j)) {
                        parameters->push(top_args->pop(j));
                    }
                }

#if VIUA_VM_DEBUG_LOG
                viua_err("[sched:vps:died:notify-watchdog] pid = ",
                         th->pid().get(),
                         ", death cause: ",
                         exc->str());
#endif

                auto death_message = make_unique<viua::types::Struct>();
                death_message->insert("function",
                                   make_unique<viua::types::Function>(
                                       th->trace().at(0)->function_name));
                auto exc =
                    th->transfer_active_exception();
                death_message->insert("exception", std::move(exc));
                death_message->insert("parameters", std::move(parameters));

                auto death_frame = make_unique<Frame>(nullptr, 1);
                death_frame->arguments->set(0, std::move(death_message));
#if VIUA_VM_DEBUG_LOG
                viua_err("[scheduler:vps:",
                         this,
                         ":watchdogging] process ",
                         th,
                         ':',
                         th->starting_function(),
                         ": died, becomes ",
                         th->watchdog());
#endif
                th->become(th->watchdog(), std::move(death_frame));
                running_processes_list.emplace_back(std::move(processes.at(i)));
                ticked = true;
            }

            continue;
        }

        // if the process stopped and is not joinable declare it dead and
        // schedule for removal thus shortening the vector of running
        // processes_list and speeding up execution
        if (th->stopped()) {
            attached_kernel->record_process_result(th);
            dead_processes_list.emplace_back(std::move(processes.at(i)));
        } else {
            running_processes_list.emplace_back(std::move(processes.at(i)));
        }
    }

    processes.erase(processes.begin(), processes.end());
    processes.swap(running_processes_list);

    // FIXME scheduler should sleep only after checking if there are no free
    // processes to run and rebalancing
    if (not any_active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return ticked;
}
void viua::scheduler::VirtualProcessScheduler::operator()() {
    while (true) {
        // FIXME perform a single burst at a time - if scheduler keeps bursting
        // for a long time some free processes may wait "forever" before being
        // migrated to a scheduler
        while (burst())
            ;

#if VIUA_VM_DEBUG_LOG
        viua_err("[scheduler:vps:", this, "] burst finished");
#endif

        // FIXME MEMORY this is accessing kernel-specific variables by pointer
        // rewrite this so it's the kernel that gives the scheduler a lock
        unique_lock<mutex> lock(*free_processes_mutex);
        // FIXME don't wait forever after single-bursting is implemented, wait
        // one time, then continue to rebalancing and just run again
        while (not free_processes_cv->wait_for(
            lock, chrono::milliseconds(10), [this] {
                auto scheduler_should_shut_down =
                    shut_down.load(std::memory_order_acquire);
                auto there_are_free_processes = (not free_processes->empty());
                return (there_are_free_processes or scheduler_should_shut_down);
            }))
            ;

        // FIXME XXX this exit condition is dubious - scheduler should exit when
        // the shut_dow is true, not when there are no free processes
        // FIXME SEGFAULT RACECONDITION what if a process has been suspended
        // because it issued a FFI call, the scheduler exits (deleting the
        // process), and then the FFI call returns - segfault
        if (free_processes->empty()) {
// this means that shutdown() was received
#if VIUA_VM_DEBUG_LOG
            viua_err("[scheduler:vps:",
                     this,
                     "] shutting down with ",
                     processes.size(),
                     " local processes");
#endif
            break;
        }

        /*
         * Determine if this scheduler should fetch processes from kernel if any
         * are available.
         *
         * The algorithm is simple: if current load is less than our fair share,
         * fetch a process.
         * Repeat until we're a good, hardworking scheduler.
         */
        const auto total_processes    = attached_kernel->pids();
        const auto running_schedulers = attached_kernel->no_of_vp_schedulers();
        /*
         * The "<=" check is *FREAKIN' IMPORTANT* because if:
         *
         *  - schedulers load is zero, and
         *  - wanted load (total processes / number of schedulers) is zero
         *
         * VM would deadlock as current load would not be *less* than wanted,
         * which is a prerequisite for fetching processes. Such a situation
         * could occur if you'd run the VM with high number of schedulers, and
         * low number of processes.
         *
         * *REMEMBER* about corner cases, or they will come back to bite you
         * when you least expect it.
         */
        while (current_load <= (total_processes / running_schedulers)
               and not free_processes->empty()) {
            processes.emplace_back(std::move(free_processes->front()));
            free_processes->erase(free_processes->begin());
            processes.back()->migrate_to(this);
#if VIUA_VM_DEBUG_LOG
            viua_err("[scheduler:vps:",
                     this,
                     ":process-grab] grabbed process ",
                     processes.back().get(),
                     ':',
                     processes.back()->starting_function());
#endif
            ++current_load;
        }
    }
#if VIUA_VM_DEBUG_LOG
    viua_err("[scheduler:vps:",
             this,
             "] shut down with ",
             processes.size(),
             " local processes");
#endif
}

void viua::scheduler::VirtualProcessScheduler::bootstrap(
    const vector<string>& commandline_arguments) {
    auto initial_frame           = make_unique<Frame>(nullptr, 0, 2);
    initial_frame->function_name = ENTRY_FUNCTION_NAME;

    auto cmdline = make_unique<viua::types::Vector>();
    auto limit   = commandline_arguments.size();
    for (decltype(limit) i = 0; i < limit; ++i) {
        cmdline->push(
            make_unique<viua::types::String>(commandline_arguments[i]));
    }
    initial_frame->local_register_set->set(1, std::move(cmdline));

    main_process = spawn(std::move(initial_frame), nullptr, true);
    main_process->priority(16);
}

void viua::scheduler::VirtualProcessScheduler::launch() {
    scheduler_thread = thread([this] { (*this)(); });
}

void viua::scheduler::VirtualProcessScheduler::shutdown() {
    shut_down.store(true, std::memory_order_release);
}

void viua::scheduler::VirtualProcessScheduler::join() {
    scheduler_thread.join();
}

int viua::scheduler::VirtualProcessScheduler::exit() const {
    return exit_code;
}

viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(
    viua::kernel::Kernel* akernel,
    vector<unique_ptr<viua::process::Process>>* fp,
    mutex* fp_mtx,
    condition_variable* fp_cv,
    const bool enable_tracing)
        : attached_kernel(akernel)
        , tracing_enabled(enable_tracing)
        , free_processes(fp)
        , free_processes_mutex(fp_mtx)
        , free_processes_cv(fp_cv)
        , main_process(nullptr)
        , current_process_index(0)
        , exit_code(0)
        , current_load(0)
        , shut_down(false) {}

viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(
    VirtualProcessScheduler&& that)
        : tracing_enabled(that.tracing_enabled) {
    attached_kernel = that.attached_kernel;

    free_processes       = that.free_processes;
    free_processes_mutex = that.free_processes_mutex;
    free_processes_cv    = that.free_processes_cv;

    main_process               = that.main_process;
    that.main_process          = nullptr;
    processes                  = std::move(that.processes);
    current_process_index      = that.current_process_index;
    that.current_process_index = 0;

    exit_code = that.exit_code;
    shut_down.store(that.shut_down.load());

    scheduler_thread = std::move(that.scheduler_thread);
}

viua::scheduler::VirtualProcessScheduler::~VirtualProcessScheduler() {}
