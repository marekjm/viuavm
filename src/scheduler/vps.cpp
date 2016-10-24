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


static void printStackTrace(Process *process) {
    auto trace = process->trace();
    cout << "stack trace: from entry point, most recent call last...\n";
    for (unsigned i = (trace.size() and trace[0]->function_name == "__entry"); i < trace.size(); ++i) {
        cout << "  " << stringifyFunctionInvocation(trace[i]) << "\n";
    }
    cout << "\n";

    unique_ptr<Type> thrown_object(process->transferActiveException());
    Exception* ex = dynamic_cast<Exception*>(thrown_object.get());
    string ex_type = thrown_object->type();

    //cout << "exception after " << kernel.counter() << " ticks" << endl;
    //cout << "failed instruction: " << get<0>(disassembler::instruction(process->executionAt())) << endl;
    cout << "uncaught object: " << ex_type << " = " << (ex ? ex->what() : thrown_object->str()) << endl;
    cout << "\n";

    cout << "frame details:\n";

    if (trace.size()) {
        Frame* last = trace.back();
        if (last->regset->size()) {
            unsigned non_empty = 0;
            for (unsigned r = 0; r < last->regset->size(); ++r) {
                if (last->regset->at(r) != nullptr) { ++non_empty; }
            }
            cout << "  non-empty registers: " << non_empty << '/' << last->regset->size();
            cout << (non_empty ? ":\n" : "\n");
            for (unsigned r = 0; r < last->regset->size(); ++r) {
                if (last->regset->at(r) == nullptr) { continue; }
                cout << "    registers[" << r << "]: ";
                cout << '<' << last->regset->get(r)->type() << "> " << last->regset->get(r)->str() << endl;
            }
        } else {
            cout << "  no registers were allocated for this frame" << endl;
        }

        if (last->args->size()) {
            cout << "  non-empty arguments (out of " << last->args->size() << "):" << endl;
            for (unsigned r = 0; r < last->args->size(); ++r) {
                if (last->args->at(r) == nullptr) { continue; }
                cout << "    arguments[" << r << "]: ";
                if (last->args->isflagged(r, MOVED)) {
                    cout << "[moved] ";
                }
                if (Pointer* ptr = dynamic_cast<Pointer*>(last->args->get(r))) {
                    if (ptr->expired()) {
                        cout << "<ExpiredPointer>" << endl;
                    } else {
                        cout << '<' << ptr->type() << '>' << endl;
                    }
                } else {
                    cout << '<' << last->args->get(r)->type() << "> " << last->args->get(r)->str() << endl;
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

template<class ...Ts> static void viua_err(Ts&&... args) {
    ostringstream oss;
    oss << std::chrono::steady_clock::now().time_since_epoch().count() << ' ';
    viua_err(oss, std::forward<Ts>(args)...);
    cerr << (oss.str() + '\n');
}


bool viua::scheduler::VirtualProcessScheduler::executeQuant(Process *th, unsigned priority) {
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

    for (unsigned j = 0; (priority == 0 or j < priority); ++j) {
        if (th->stopped()) {
            // remember to break if the process stopped
            // otherwise the Kernel will try to tick the process and
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

Kernel* viua::scheduler::VirtualProcessScheduler::kernel() const {
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

void viua::scheduler::VirtualProcessScheduler::registerPrototype(Prototype *proto) {
    attached_kernel->registerPrototype(proto);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignFunctionCall(Frame *frame, Process *p) const {
    attached_kernel->requestForeignFunctionCall(frame, p);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignMethodCall(const string& name, Type *object, Frame *frame, RegisterSet*, RegisterSet*, Process *p) {
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

Process* viua::scheduler::VirtualProcessScheduler::process(decltype(processes)::size_type index) {
    return processes.at(index).get();
}

Process* viua::scheduler::VirtualProcessScheduler::process() {
    return process(current_process_index);
}

Process* viua::scheduler::VirtualProcessScheduler::spawn(unique_ptr<Frame> frame, Process *parent, bool disown) {
    unique_ptr<Process> p(new Process(std::move(frame), this, parent));
    p->begin();
    if (disown) {
        p->detach();
    }

    Process *process_ptr = p.get();
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
        viua_err( "[scheduler:vps:", this, "] posting process ", p.get(), ":", p->starting_function(), " to kernel");
        attached_kernel->postFreeProcess(std::move(p));
    } else {
        viua_err( "[scheduler:vps:", this, "] retaining process ", p.get(), ":", p->starting_function());
        processes.emplace_back(std::move(p));
        retained_process = true;
    }

    return process_ptr;
}

void viua::scheduler::VirtualProcessScheduler::spawnWatchdog(unique_ptr<Frame> frame) {
    if (watchdog_process) {
        throw new Exception("watchdog process already spawned");
    }
    watchdog_function = frame->function_name;
    watchdog_process.reset(new Process(std::move(frame), this, nullptr));
#if VIUA_VM_DEBUG_LOG
    viua_err( "[sched:vps:watchdog:spawn] pid = ", watchdog_process->pid().get());
#endif
    watchdog_process->hidden(true);
    watchdog_process->begin();
}

void viua::scheduler::VirtualProcessScheduler::resurrectWatchdog() {
    auto active_exception = watchdog_process->getActiveException();
    if (active_exception) {
        cout << "watchdog process terminated by: " << active_exception->type() << ": '" << active_exception->str() << "'" << endl;
    }

    watchdog_process.reset(nullptr);

    unique_ptr<Frame> frm(new Frame(nullptr, 0, 64));
    frm->function_name = watchdog_function;
    spawnWatchdog(std::move(frm));
}

void viua::scheduler::VirtualProcessScheduler::send(const PID pid, unique_ptr<Type> message) {
#if VIUA_VM_DEBUG_LOG
    viua_err( "[sched:vps:send] pid = ", pid.get());
#endif
    attached_kernel->send(pid, std::move(message));
}

void viua::scheduler::VirtualProcessScheduler::receive(const PID pid, queue<unique_ptr<Type>>& message_queue) {
#if VIUA_VM_DEBUG_LOG
    viua_err( "[sched:vps:receive] pid = ", pid.get());
#endif
    attached_kernel->receive(pid, message_queue);
}

bool viua::scheduler::VirtualProcessScheduler::burst() {
    if (not processes.size()) {
        // make Kernel stop if there are no processes_list to run
        return false;
    }

    bool ticked = false;

    vector<unique_ptr<Process>> running_processes_list;
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
            if (not watchdog_process) {
                if (th == main_process) {
                    exit_code = 1;
                }

                auto trace = th->trace();

                ostringstream errss;
                viua_err( errss, "process ", current_process_index, " spawned using ");
                if (trace.size() > 1) {
                    // if trace size if greater than one, detect if this is main process
                    viua_err( errss, trace[(trace[0]->function_name == ENTRY_FUNCTION_NAME)]->function_name);
                } else if (trace.size() == 1) {
                    // if trace size is equal to one, just print the top-most function
                    viua_err( errss, trace[0]->function_name);
                } else {
                    // in all other cases print the function the process has been started with
                    // it is a safe bet (perhaps even safer than printing the top-most function on
                    // the stack as that may have been changed by a tail call...)
                    viua_err( errss, th->starting_function());
                }
                viua_err( errss, " has terminated");
                cerr << (errss.str() + '\n');

                printStackTrace(th);
            } else {
                Object* death_message = new Object("Object");
                unique_ptr<Type> exc(th->transferActiveException());
                Vector *parameters = new Vector();
                RegisterSet *top_args = th->trace()[0]->args;
                for (unsigned long j = 0; j < top_args->size(); ++j) {
                    if (top_args->at(j)) {
                        parameters->push(top_args->at(j));
                    }
                }
                top_args->drop();

#if VIUA_VM_DEBUG_LOG
                viua_err( "[sched:vps:died:notify-watchdog] pid = ", th->pid().get(), ", death cause: ", exc->str());
#endif

                death_message->set("function", new Function(th->trace()[0]->function_name));
                death_message->set("exception", exc.release());
                death_message->set("parameters", parameters);

            attached_kernel->deleteMailbox(th->pid());
            // push broken process to dead processes_list list to
            // erase it later
            dead_processes_list.emplace_back(std::move(processes.at(i)));

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

    viua_err("[scheduler:vps:", this, "] processes = ", processes.size(), ", running processes = ", running_processes_list.size(), ", dead processes = ", dead_processes_list.size());
    if (retained_process) {
        viua_err( "[scheduler:vps:", this, "] retained one or more processes");
    }
    processes.erase(processes.begin(), processes.end());
    processes.swap(running_processes_list);

    while (watchdog_process and not watchdog_process->suspended() and not watchdog_process->empty()) {
        executeQuant(watchdog_process.get(), 0);
        if (watchdog_process->terminated() or watchdog_process->stopped()) {
            resurrectWatchdog();
        }
    }

    bool continue_running = (ticked or retained_process);
    if (continue_running and retained_process) {
        viua_err("[scheduler:vps:", this, "] continue running after retaining one or more processes");
    }
    retained_process = false;
    return continue_running;
}
void viua::scheduler::VirtualProcessScheduler::operator()() {
    while (true) {
        while (burst());

        viua_err("[scheduler:vps:", this, "] burst finished");

        unique_lock<mutex> lock(*free_processes_mutex);
        while (not free_processes_cv->wait_for(lock, chrono::milliseconds(10), [this]{
            return (not free_processes->empty() or shut_down.load(std::memory_order_acquire));
        }));

        if (free_processes->empty()) {
            // this means that shutdown() was received
            viua_err( "[scheduler:vps:", this, "] shutting down with ", processes.size(), " local processes");
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
            viua_err("[scheduler:vps:", this, ":process-grab] grabbed process ", processes.back().get(), ':', processes.back()->starting_function());
            ++current_load;
        }
    }
    viua_err( "[scheduler:vps:", this, "] shut down with ", processes.size(), " local processes");
}

void viua::scheduler::VirtualProcessScheduler::bootstrap(const vector<string>& commandline_arguments) {
    unique_ptr<Frame> initial_frame(new Frame(nullptr, 0, 2));
    initial_frame->function_name = ENTRY_FUNCTION_NAME;

    Vector* cmdline = new Vector();
    auto limit = commandline_arguments.size();
    for (decltype(limit) i = 0; i < limit; ++i) {
        cmdline->push(new String(commandline_arguments[i]));
    }
    initial_frame->regset->set(1, cmdline);

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

viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(Kernel *akernel, vector<unique_ptr<Process>> *fp,
                                                                  mutex *fp_mtx,
                                                                  condition_variable *fp_cv):
    attached_kernel(akernel),
    free_processes(fp),
    free_processes_mutex(fp_mtx),
    free_processes_cv(fp_cv),
    main_process(nullptr),
    current_process_index(0),
    watchdog_process(nullptr),
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

    watchdog_function = that.watchdog_function;
    watchdog_process = std::move(that.watchdog_process);

    exit_code = that.exit_code;
    shut_down.store(that.shut_down.load());

    scheduler_thread = std::move(that.scheduler_thread);
}

viua::scheduler::VirtualProcessScheduler::~VirtualProcessScheduler() {}
