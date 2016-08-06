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
#include <viua/machine.h>
#include <viua/printutils.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/types/function.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/process.h>
#include <viua/cpu/cpu.h>
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

    //cout << "exception after " << cpu.counter() << " ticks" << endl;
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
            // otherwise the CPU will try to tick the process and
            // it will crash (will try to execute instructions from 0x0 pointer)
            break;
        }
        if (th->suspended()) {
            // do not execute suspended processes
            break;
        }
        th->tick();
    }

    return true;
}

CPU* viua::scheduler::VirtualProcessScheduler::cpu() const {
    return attached_cpu;
}

bool viua::scheduler::VirtualProcessScheduler::isClass(const string& name) const {
    return attached_cpu->isClass(name);
}

bool viua::scheduler::VirtualProcessScheduler::classAccepts(const string& klass, const string& method) const {
    return attached_cpu->classAccepts(klass, method);
}

auto viua::scheduler::VirtualProcessScheduler::inheritanceChainOf(const std::string& name) const -> decltype(attached_cpu->inheritanceChainOf(name)) {
    return attached_cpu->inheritanceChainOf(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLocalFunction(const string& name) const {
    return attached_cpu->isLocalFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLinkedFunction(const string& name) const {
    return attached_cpu->isLinkedFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isNativeFunction(const string& name) const {
    return attached_cpu->isNativeFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isForeignMethod(const string& name) const {
    return attached_cpu->isForeignMethod(name);
}

bool viua::scheduler::VirtualProcessScheduler::isForeignFunction(const string& name) const {
    return attached_cpu->isForeignFunction(name);
}

bool viua::scheduler::VirtualProcessScheduler::isBlock(const string& name) const {
    return attached_cpu->isBlock(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLocalBlock(const string& name) const {
    return attached_cpu->isLocalBlock(name);
}

bool viua::scheduler::VirtualProcessScheduler::isLinkedBlock(const string& name) const {
    return attached_cpu->isLinkedBlock(name);
}

pair<byte*, byte*> viua::scheduler::VirtualProcessScheduler::getEntryPointOfBlock(const std::string& name) const {
    return attached_cpu->getEntryPointOfBlock(name);
}

string viua::scheduler::VirtualProcessScheduler::resolveMethodName(const string& klass, const string& method) const {
    return attached_cpu->resolveMethodName(klass, method);
}

pair<byte*, byte*> viua::scheduler::VirtualProcessScheduler::getEntryPointOf(const std::string& name) const {
    return attached_cpu->getEntryPointOf(name);
}

void viua::scheduler::VirtualProcessScheduler::registerPrototype(Prototype *proto) {
    attached_cpu->registerPrototype(proto);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignFunctionCall(Frame *frame, Process *p) const {
    attached_cpu->requestForeignFunctionCall(frame, p);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignMethodCall(const string& name, Type *object, Frame *frame, RegisterSet*, RegisterSet*, Process *p) {
    attached_cpu->requestForeignMethodCall(name, object, frame, nullptr, nullptr, p);
}

void viua::scheduler::VirtualProcessScheduler::loadNativeLibrary(const string& name) {
    attached_cpu->loadNativeLibrary(name);
}

void viua::scheduler::VirtualProcessScheduler::loadForeignLibrary(const string& name) {
    attached_cpu->loadForeignLibrary(name);
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

Process* viua::scheduler::VirtualProcessScheduler::spawn(unique_ptr<Frame> frame, Process *parent) {
    unique_ptr<Process> p(new Process(std::move(frame), this, parent));
    p->begin();
    processes.push_back(std::move(p));
    return processes.back().get();
}

void viua::scheduler::VirtualProcessScheduler::spawnWatchdog(unique_ptr<Frame> frame) {
    if (watchdog_process) {
        throw new Exception("watchdog process already spawned");
    }
    watchdog_function = frame->function_name;
    watchdog_process.reset(new Process(std::move(frame), this, nullptr));
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

bool viua::scheduler::VirtualProcessScheduler::burst() {
    if (not processes.size()) {
        // make CPU stop if there are no processes_list to run
        return false;
    }

    bool ticked = false;

    vector<unique_ptr<Process>> running_processes_list;
    decltype(running_processes_list) dead_processes_list;
    for (decltype(running_processes_list)::size_type i = 0; i < processes.size(); ++i) {
        current_process_index = i;
        auto th = processes.at(i).get();

        ticked = (executeQuant(th, th->priority()) or ticked);

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
            running_processes_list.push_back(std::move(processes.at(i)));
            continue;
        }

        if (th->terminated() and not th->joinable() and th->parent() == nullptr) {
            if (not watchdog_process) {
                if (th == main_process) {
                    exit_code = 1;
                }

                auto trace = th->trace();

                cout << "process " << current_process_index << " spawned using ";
                if (trace.size() > 1) {
                    // if trace size if greater than one, detect if this is main process
                    cout << trace[(trace[0]->function_name == ENTRY_FUNCTION_NAME)]->function_name;
                } else if (trace.size()) {
                    // if trace size is equal to one, just print the top-most function
                    cout << trace[0]->function_name;
                } else {
                    cout << "<function unavailable>";
                }
                cout << " has terminated\n";
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
                death_message->set("function", new Function(th->trace()[0]->function_name));
                death_message->set("exception", exc.release());
                death_message->set("parameters", parameters);
                watchdog_process->pass(unique_ptr<Type>(death_message));
            }

            // push broken process to dead processes_list list to
            // erase it later
            dead_processes_list.push_back(std::move(processes.at(i)));

            continue;
        }

        // if the process stopped and is not joinable declare it dead and
        // schedule for removal thus shortening the vector of running processes_list and
        // speeding up execution
        if (th->stopped() and (not th->joinable())) {
            dead_processes_list.push_back(std::move(processes.at(i)));
        } else {
            running_processes_list.push_back(std::move(processes.at(i)));
        }
    }

    processes.erase(processes.begin(), processes.end());
    processes.swap(running_processes_list);

    while (watchdog_process and not watchdog_process->suspended()) {
        executeQuant(watchdog_process.get(), 0);
        if (watchdog_process->terminated() or watchdog_process->stopped()) {
            resurrectWatchdog();
        }
    }

    return ticked;
}
bool viua::scheduler::VirtualProcessScheduler::operator()() {
    return burst();
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

    main_process = spawn(std::move(initial_frame), nullptr);
    main_process->detach();
    main_process->priority(16);
}

int viua::scheduler::VirtualProcessScheduler::exit() const {
    return exit_code;
}

viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(CPU *acpu):
    attached_cpu(acpu),
    main_process(nullptr),
    current_process_index(0),
    watchdog_process(nullptr),
    exit_code(0)
{
}

viua::scheduler::VirtualProcessScheduler::~VirtualProcessScheduler() {}
