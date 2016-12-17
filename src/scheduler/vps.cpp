/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#include <vector>
#include <string>
#include <sstream>
#include <viua/machine.h>
#include <viua/printutils.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/types/function.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/process.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


static void printStackTrace(viua::process::Process *process) {
    auto trace = process->trace();
    cout << "stack trace: from entry point, most recent call last...\n";
    for (decltype(trace)::size_type i = (trace.size() and trace[0]->function_name == "__entry"); i < trace.size(); ++i) {
        cout << "  " << stringifyFunctionInvocation(trace[i]) << "\n";
    }
    cout << "\n";

    unique_ptr<viua::types::Type> thrown_object(process->transferActiveException());
    auto ex = dynamic_cast<viua::types::Exception*>(thrown_object.get());
    string ex_type = thrown_object->type();

    //cout << "exception after " << kernel.counter() << " ticks" << endl;
    //cout << "failed instruction: " << get<0>(disassembler::instruction(process->executionAt())) << endl;
    cout << "uncaught object: " << ex_type << " = " << (ex ? ex->what() : thrown_object->str()) << endl;
    cout << "\n";

    cout << "frame details:\n";

    if (trace.size()) {
        Frame* last = trace.back();
        if (last->local_register_set->size()) {
            unsigned non_empty = 0;
            for (decltype(last->local_register_set->size()) r = 0; r < last->local_register_set->size(); ++r) {
                if (last->local_register_set->at(r) != nullptr) { ++non_empty; }
            }
            cout << "  non-empty registers: " << non_empty << '/' << last->local_register_set->size();
            cout << (non_empty ? ":\n" : "\n");
            for (decltype(last->local_register_set->size()) r = 0; r < last->local_register_set->size(); ++r) {
                if (last->local_register_set->at(r) == nullptr) { continue; }
                cout << "    registers[" << r << "]: ";
                cout << '<' << last->local_register_set->get(r)->type() << "> " << last->local_register_set->get(r)->str() << endl;
            }
        } else {
            cout << "  no registers were allocated for this frame" << endl;
        }

        if (last->arguments->size()) {
            cout << "  non-empty arguments (out of " << last->arguments->size() << "):" << endl;
            for (decltype(last->arguments->size()) r = 0; r < last->arguments->size(); ++r) {
                if (last->arguments->at(r) == nullptr) { continue; }
                cout << "    arguments[" << r << "]: ";
                if (last->arguments->isflagged(r, MOVED)) {
                    cout << "[moved] ";
                }
                if (auto ptr = dynamic_cast<viua::types::Pointer*>(last->arguments->get(r))) {
                    if (ptr->expired()) {
                        cout << "<ExpiredPointer>" << endl;
                    } else {
                        cout << '<' << ptr->type() << '>' << endl;
                    }
                } else {
                    cout << '<' << last->arguments->get(r)->type() << "> " << last->arguments->get(r)->str() << endl;
                }
            }
        } else {
            cout << "  no arguments were passed to this frame" << endl;
        }
    } else {
        cout << "no stack trace available" << endl;
    }
}

template<class T> static void viua_err(
    ostringstream& oss,
    T&& arg
) {
    oss << arg;
}
template<class T, class ...Ts> static void viua_err(
    ostringstream& oss,
    T&& arg,
    Ts&&... Rest
) {
    oss << arg;
    viua_err(oss, std::forward<Ts>(Rest)...);
}

template<class ...Ts> static void viua_err(Ts&&... arguments) {
    ostringstream oss;
    oss << std::chrono::steady_clock::now().time_since_epoch().count() << ' ';
    viua_err(oss, std::forward<Ts>(arguments)...);
    cerr << (oss.str() + '\n');
}


bool viua::scheduler::VirtualProcessScheduler::executeQuant(viua::process::Process *th, uint16_t priority) {
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
        viua_err( "[sched:vps:quant] pid = ", th->pid().get(), ", tick = ", j);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        th->tick();
    }

    return true;
}

viua::kernel::Kernel* viua::scheduler::VirtualProcessScheduler::kernel() const {
    return attached_kernel;
}

bool viua::scheduler::VirtualProcessScheduler::isClass(const string& name) const {
    return attached_kernel->isClass(name);
}

bool viua::scheduler::VirtualProcessScheduler::classAccepts(const string& klass, const string& method) const {
    return attached_kernel->classAccepts(klass, method);
}

auto viua::scheduler::VirtualProcessScheduler::inheritanceChainOf(const std::string& name) const -> decltype(attached_kernel->inheritanceChainOf(name)) {
    return attached_kernel->inheritanceChainOf(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLocalFunction(const string& name) const {
    return attached_kernel->isLocalFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLinkedFunction(const string& name) const {
    return attached_kernel->isLinkedFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isNativeFunction(const string& name) const {
    return attached_kernel->isNativeFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isForeignMethod(const string& name) const {
    return attached_kernel->isForeignMethod(name);
}

bool viua::scheduler::VirtualProcessScheduler::isForeignFunction(const string& name) const {
    return attached_kernel->isForeignFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isBlock(const string& name) const {
    return attached_kernel->isBlock(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLocalBlock(const string& name) const {
    return attached_kernel->isLocalBlock(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLinkedBlock(const string& name) const {
    return attached_kernel->isLinkedBlock(name);
}

pair<byte*, byte*> viua::scheduler::VirtualProcessScheduler::getEntryPointOfBlock(const std::string& name) const {
    return attached_kernel->getEntryPointOfBlock(name);
}

string viua::scheduler::VirtualProcessScheduler::resolveMethodName(const string& klass, const string& method) const {
    return attached_kernel->resolveMethodName(klass, method);
}

pair<byte*, byte*> viua::scheduler::VirtualProcessScheduler::getEntryPointOf(const std::string& name) const {
    return attached_kernel->getEntryPointOf(name);
}

void viua::scheduler::VirtualProcessScheduler::registerPrototype(unique_ptr<viua::types::Prototype> proto) {
    attached_kernel->registerPrototype(std::move(proto));
}

void viua::scheduler::VirtualProcessScheduler::requestForeignFunctionCall(Frame *frame, viua::process::Process *p) const {
    attached_kernel->requestForeignFunctionCall(frame, p);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignMethodCall(const string& name, viua::types::Type *object, Frame *frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process *p) {
    attached_kernel->requestForeignMethodCall(name, object, frame, nullptr, nullptr, p);
}

void viua::scheduler::VirtualProcessScheduler::loadNativeLibrary(const string& name) {
    attached_kernel->loadNativeLibrary(name);
}

void viua::scheduler::VirtualProcessScheduler::loadForeignLibrary(const string& name) {
    attached_kernel->loadForeignLibrary(name);
}

auto viua::scheduler::VirtualProcessScheduler::cpi() const -> decltype(processes)::size_type {
    return current_process_index;
}


auto viua::scheduler::VirtualProcessScheduler::size() const -> decltype(processes)::size_type {
    return processes.size();
}

viua::process::Process* viua::scheduler::VirtualProcessScheduler::process(decltype(processes)::size_type index) {
    return processes.at(index).get();
}

viua::process::Process* viua::scheduler::VirtualProcessScheduler::process() {
    return process(current_process_index);
}

viua::process::Process* viua::scheduler::VirtualProcessScheduler::spawn(unique_ptr<Frame> frame, viua::process::Process *parent, bool disown) {
    unique_ptr<viua::process::Process> p(new viua::process::Process(std::move(frame), this, parent));
    p->begin();
    if (disown) {
        p->detach();
    }

    viua::process::Process *process_ptr = p.get();
    const auto total_processes = attached_kernel->createMailbox(process_ptr->pid());
    const auto running_schedulers = attached_kernel->no_of_vp_schedulers();

    /*
     * Determine if this scheduler is overburdened and should post processes to kernel.
     *
     * The algorithm is simple: if current load is slightly more than our fair share,
     * post spawned process to kernel and let other schedulers handle it.
     * The "slightly more" part is there because we try to be a good scheduler and
     * don't just push our processes to others as soon as we're a little bit tired.
     * Also, because the processes may be short-lived and the costs of synchronisation
     * may outweight the benefits of increased concurrency.
     *
     * Benchmarks proved that dealing locally with increased load for a period of time
     * before posting processes to kernel is a better solution than posting immediately.
     * This was measured using lots of short-lived processes in the "N bottles of beer"
     * bechmark: 16384 bottles, 8 schedulers, on a CPU with 4 physical cores.
     */
    if (processes.size() > ((total_processes / running_schedulers) / 100 * 140)) {
#if VIUA_VM_DEBUG_LOG
        viua_err( "[scheduler:vps:", this, "] posting process ", p.get(), ":", p->starting_function(), " to kernel");
#endif
        attached_kernel->postFreeProcess(std::move(p));
    } else {
#if VIUA_VM_DEBUG_LOG
        viua_err( "[scheduler:vps:", this, "] retaining process ", p.get(), ":", p->starting_function());
#endif
        processes.emplace_back(std::move(p));
    }

    return process_ptr;
}

void viua::scheduler::VirtualProcessScheduler::send(const viua::process::PID pid, unique_ptr<viua::types::Type> message) {
#if VIUA_VM_DEBUG_LOG
    viua_err( "[sched:vps:send] pid = ", pid.get());
#endif
    attached_kernel->send(pid, std::move(message));
}

void viua::scheduler::VirtualProcessScheduler::receive(const viua::process::PID pid, queue<unique_ptr<viua::types::Type>>& message_queue) {
#if VIUA_VM_DEBUG_LOG
    viua_err( "[sched:vps:receive] pid = ", pid.get());
#endif
    attached_kernel->receive(pid, message_queue);
}

bool viua::scheduler::VirtualProcessScheduler::burst() {
    if (not processes.size()) {
        // make kernel stop if there are no processes_list to run
        return false;
    }

    bool ticked = false;

    vector<unique_ptr<viua::process::Process>> running_processes_list;
    decltype(running_processes_list) dead_processes_list;
    current_load = 0;
    for (decltype(running_processes_list)::size_type i = 0; i < processes.size(); ++i) {
        current_process_index = i;
        auto th = processes.at(i).get();

#if VIUA_VM_DEBUG_LOG
        viua_err( "[sched:vps:burst] pid = ", th->pid().get());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        executeQuant(th, th->priority());
        ticked = (ticked or (not th->stopped()) or th->suspended());

        if (not (th->suspended() or th->terminated() or th->stopped())) {
            ++current_load;
        }

        if (th->suspended()) {
            // This check is required to avoid race condition later in the function.
            // When a process is suspended its state cannot really be detected correctly except
            // for the fact that the process is still possibly running.
            //
            // Race condition arises when an exception is thrown during an FFI call;
            // as FFI results and exceptions are transferred back asynchronously a following sequence
            // of events may occur:
            //
            //  - process requests FFI call, and is suspended
            //  - an exception is thrown during FFI call
            //  - the exception is registered in a process, and the process enters "terminated" state
            //  - the process is woken up, and enters "stopped" state
            //
            // Now, if the process is woken up between the next if (the `if (th-terminated() ...)`), and
            // the `if (th->stopped() ...)` one it will be marked as dead without handling the exception
            // because when the VPS was checking for presence of an exception it was not there yet.
            //
            // Checking if the process is suspended here and, if it is, immediately marking it as
            // running prevents the race condition.
            //
            // REMEMBER: the last thing that is done after servicing an FFI call is waking the process up so
            // as long as the process is suspended it must be considered to be running.
            running_processes_list.emplace_back(std::move(processes.at(i)));
            continue;
        }

        if (th->terminated() and not th->joinable() and th->parent() == nullptr) {
#if VIUA_VM_DEBUG_LOG
            viua_err( "[sched:vps:died] pid = ", th->pid().get());
#endif
            if (not th->watchdogged()) {
                if (th == main_process) {
                    exit_code = 1;
                }

                auto trace = th->trace();

                ostringstream errss;
#if VIUA_VM_DEBUG_LOG
                viua_err( errss, "process ", current_process_index, " spawned using ");
#endif
                if (trace.size() > 1) {
                    // if trace size if greater than one, detect if this is main process
#if VIUA_VM_DEBUG_LOG
                    viua_err( errss, trace[(trace[0]->function_name == ENTRY_FUNCTION_NAME)]->function_name);
#endif
                } else if (trace.size() == 1) {
                    // if trace size is equal to one, just print the top-most function
#if VIUA_VM_DEBUG_LOG
                    viua_err( errss, trace[0]->function_name);
#endif
                } else {
                    // in all other cases print the function the process has been started with
                    // it is a safe bet (perhaps even safer than printing the top-most function on
                    // the stack as that may have been changed by a tail call...)
#if VIUA_VM_DEBUG_LOG
                    viua_err( errss, th->starting_function());
#endif
                }
#if VIUA_VM_DEBUG_LOG
                viua_err( errss, " has terminated");
#endif
                cerr << (errss.str() + '\n');

                printStackTrace(th);

                attached_kernel->deleteMailbox(th->pid());
                // push broken process to dead processes_list list to
                // erase it later
#if VIUA_VM_DEBUG_LOG
                viua_err( "[scheduler:vps] process ", processes.at(i).get(), ": marked as dead");
#endif
                dead_processes_list.emplace_back(std::move(processes.at(i)));
            } else {
                unique_ptr<viua::types::Object> death_message(new viua::types::Object("Object"));
                unique_ptr<viua::types::Type> exc(th->transferActiveException());
                unique_ptr<viua::types::Vector> parameters {new viua::types::Vector()};
                viua::kernel::RegisterSet *top_args = th->trace().at(0)->arguments.get();
                for (decltype(top_args->size()) j = 0; j < top_args->size(); ++j) {
                    if (top_args->at(j)) {
                        parameters->push(top_args->pop(j));
                    }
                }

#if VIUA_VM_DEBUG_LOG
                viua_err( "[sched:vps:died:notify-watchdog] pid = ", th->pid().get(), ", death cause: ", exc->str());
#endif

                death_message->set("function", unique_ptr<viua::types::Type>{new viua::types::Function(th->trace().at(0)->function_name)});
                death_message->set("exception", std::move(exc));
                death_message->set("parameters", std::move(parameters));

                unique_ptr<Frame> death_frame(new Frame(nullptr, 1));
                death_frame->arguments->set(0, std::move(death_message));
#if VIUA_VM_DEBUG_LOG
                viua_err( "[scheduler:vps:", this, ":watchdogging] process ", th, ':', th->starting_function(), ": died, becomes ", th->watchdog());
#endif
                th->become(th->watchdog(), std::move(death_frame));
                running_processes_list.emplace_back(std::move(processes.at(i)));
                ticked = true;
            }

            continue;
        }

        // if the process stopped and is not joinable declare it dead and
        // schedule for removal thus shortening the vector of running processes_list and
        // speeding up execution
        if (th->stopped() and (not th->joinable())) {
            attached_kernel->deleteMailbox(processes.at(i)->pid());
#if VIUA_VM_DEBUG_LOG
            viua_err( "[sched:vps:", this, "] process ", th, " marked as dead: ", th->starting_function());
#endif
            dead_processes_list.emplace_back(std::move(processes.at(i)));
        } else {
            running_processes_list.emplace_back(std::move(processes.at(i)));
        }
    }

    processes.erase(processes.begin(), processes.end());
    processes.swap(running_processes_list);

    return ticked;
}
void viua::scheduler::VirtualProcessScheduler::operator()() {
    while (true) {
        while (burst());

#if VIUA_VM_DEBUG_LOG
        viua_err("[scheduler:vps:", this, "] burst finished");
#endif

        unique_lock<mutex> lock(*free_processes_mutex);
        while (not free_processes_cv->wait_for(lock, chrono::milliseconds(10), [this]{
            return (not free_processes->empty() or shut_down.load(std::memory_order_acquire));
        }));

        if (free_processes->empty()) {
            // this means that shutdown() was received
#if VIUA_VM_DEBUG_LOG
            viua_err( "[scheduler:vps:", this, "] shutting down with ", processes.size(), " local processes");
#endif
            break;
        }

        /*
         * Determine if this scheduler should fetch processes from kernel if any
         * are available.
         *
         * The algorithm is simple: if current load is less than our fair share,
         * fetch a process.
         * Repear until we're a good, hardworking scheduler.
         */
        const auto total_processes = attached_kernel->pids();
        const auto running_schedulers = attached_kernel->no_of_vp_schedulers();
        /*
         * The "<=" check is *FREAKIN' IMPORTANT* because if:
         *
         *  - schedulers load is zero, and
         *  - wanted load (total processes / number of schedulers) is zero
         *
         * VM would deadlock as current load would not be *less* than wanted, which
         * is a prerequisite for fetching processes.
         * Such a situation could occur if you'd run the VM with high number of
         * schedulers, and low number of processes.
         *
         * *REMEMBER* about corner cases, or they will come back to bite you when
         * you least expect it.
         */
        while (current_load <= (total_processes / running_schedulers) and not free_processes->empty()) {
            processes.emplace_back(std::move(free_processes->front()));
            free_processes->erase(free_processes->begin());
            processes.back()->migrate_to(this);
#if VIUA_VM_DEBUG_LOG
            viua_err("[scheduler:vps:", this, ":process-grab] grabbed process ", processes.back().get(), ':', processes.back()->starting_function());
#endif
            ++current_load;
        }
    }
#if VIUA_VM_DEBUG_LOG
    viua_err( "[scheduler:vps:", this, "] shut down with ", processes.size(), " local processes");
#endif
}

void viua::scheduler::VirtualProcessScheduler::bootstrap(const vector<string>& commandline_arguments) {
    unique_ptr<Frame> initial_frame(new Frame(nullptr, 0, 2));
    initial_frame->function_name = ENTRY_FUNCTION_NAME;

    unique_ptr<viua::types::Vector> cmdline {new viua::types::Vector()};
    auto limit = commandline_arguments.size();
    for (decltype(limit) i = 0; i < limit; ++i) {
        cmdline->push(unique_ptr<viua::types::Type>{new viua::types::String(commandline_arguments[i])});
    }
    initial_frame->local_register_set->set(1, std::move(cmdline));

    main_process = spawn(std::move(initial_frame), nullptr, true);
    main_process->priority(16);
}

void viua::scheduler::VirtualProcessScheduler::launch() {
    scheduler_thread = thread([this]{ (*this)(); });
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

viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(viua::kernel::Kernel *akernel, vector<unique_ptr<viua::process::Process>> *fp,
                                                                  mutex *fp_mtx,
                                                                  condition_variable *fp_cv):
    attached_kernel(akernel),
    free_processes(fp),
    free_processes_mutex(fp_mtx),
    free_processes_cv(fp_cv),
    main_process(nullptr),
    current_process_index(0),
    exit_code(0),
    current_load(0),
    shut_down(false)
{
}

viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(VirtualProcessScheduler&& that) {
    attached_kernel = that.attached_kernel;

    free_processes = that.free_processes;
    free_processes_mutex = that.free_processes_mutex;
    free_processes_cv = that.free_processes_cv;

    main_process = that.main_process;
    that.main_process = nullptr;
    processes = std::move(that.processes);
    current_process_index = that.current_process_index;
    that.current_process_index = 0;

    exit_code = that.exit_code;
    shut_down.store(that.shut_down.load());

    scheduler_thread = std::move(that.scheduler_thread);
}

viua::scheduler::VirtualProcessScheduler::~VirtualProcessScheduler() {}
