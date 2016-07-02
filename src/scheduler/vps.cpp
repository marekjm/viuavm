#include <vector>
#include <string>
#include <viua/machine.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/types/function.h>
#include <viua/process.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


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

auto viua::scheduler::VirtualProcessScheduler::inheritanceChainOf(const std::string& name) const -> decltype(attached_cpu->inheritanceChainOf(name)) {
    return attached_cpu->inheritanceChainOf(name);
}

bool viua::scheduler::VirtualProcessScheduler::isForeignMethod(const string& name) const {
    return attached_cpu->isForeignMethod(name);
}

bool viua::scheduler::VirtualProcessScheduler::isForeignFunction(const string& name) const {
    return attached_cpu->isForeignFunction(name);
}

pair<byte*, byte*> viua::scheduler::VirtualProcessScheduler::getEntryPointOf(const std::string& name) const {
    return attached_cpu->getEntryPointOf(name);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignFunctionCall(Frame *frame, Process *p) const {
    attached_cpu->requestForeignFunctionCall(frame, p);
}

void viua::scheduler::VirtualProcessScheduler::requestForeignMethodCall(const string& name, Type *object, Frame *frame, RegisterSet*, RegisterSet*, Process *p) {
    attached_cpu->requestForeignMethodCall(name, object, frame, nullptr, nullptr, p);
}

auto viua::scheduler::VirtualProcessScheduler::cpi() const -> decltype(processes)::size_type {
    return current_process_index;
}

Process* viua::scheduler::VirtualProcessScheduler::process(decltype(processes)::size_type index) {
    return procs->at(index);
}

Process* viua::scheduler::VirtualProcessScheduler::process() {
    return process(current_process_index);
}

bool viua::scheduler::VirtualProcessScheduler::burst() {
    if (not procs->size()) {
        // make CPU stop if there are no processes_list to run
        return false;
    }

    bool ticked = false;

    vector<Process*> running_processes_list;
    decltype(running_processes_list) dead_processes_list;
    bool abort_because_of_process_termination = false;
    for (decltype(running_processes_list)::size_type i = 0; i < procs->size(); ++i) {
        current_process_index = i;
        auto th = procs->at(i);

        ticked = (executeQuant(th, th->priority()) or ticked);

        if (th->terminated() and not th->joinable() and th->parent() == nullptr) {
            if (attached_cpu->currentWatchdog() == nullptr) {
                abort_because_of_process_termination = true;
            } else {
                Object* death_message = new Object("Object");
                Type* exc = th->transferActiveException();
                Vector *parameters = new Vector();
                RegisterSet *top_args = th->trace()[0]->args;
                for (unsigned long j = 0; j < top_args->size(); ++j) {
                    if (top_args->at(j)) {
                        parameters->push(top_args->at(j));
                    }
                }
                top_args->drop();
                death_message->set("function", new Function(th->trace()[0]->function_name));
                death_message->set("exception", exc);
                death_message->set("parameters", parameters);
                attached_cpu->currentWatchdog()->pass(death_message);

                // push broken process to dead processes_list list to
                // erase it later
                dead_processes_list.push_back(th);

                // copy what is left after this broken process to running processes_list array to
                // prevent losing them
                // because after the loop the `processes_list` vector is reset
                for (decltype(running_processes_list)::size_type j = i+1; j < procs->size(); ++j) {
                    running_processes_list.push_back(procs->at(j));
                }
            }
            break;
        }

        // if the process stopped and is not joinable declare it dead and
        // schedule for removal thus shortening the vector of running processes_list and
        // speeding up execution
        if (th->stopped() and (not th->joinable())) {
            dead_processes_list.push_back(th);
        } else {
            running_processes_list.push_back(th);
        }
    }

    if (abort_because_of_process_termination) {
        return false;
    }

    for (decltype(dead_processes_list)::size_type i = 0; i < dead_processes_list.size(); ++i) {
        delete dead_processes_list[i];
    }

    // if there were any dead processes_list we must rebuild the scheduled processes_list vector
    if (dead_processes_list.size()) {
        procs->erase(procs->begin(), procs->end());
        for (decltype(running_processes_list)::size_type i = 0; i < running_processes_list.size(); ++i) {
            procs->push_back(running_processes_list[i]);
        }
    }

    Process *watchdog_process = attached_cpu->currentWatchdog();
    while (watchdog_process != nullptr and not watchdog_process->suspended()) {
        executeQuant(watchdog_process, 0);
        watchdog_process = attached_cpu->currentWatchdog();
        if (watchdog_process->terminated() or watchdog_process->stopped()) {
            attached_cpu->resurrectWatchdog();
        }
    }

    return ticked;
}

void viua::scheduler::VirtualProcessScheduler::bootstrap(const vector<string>& commandline_arguments, byte *jump_base) {
    Frame *initial_frame = new Frame(nullptr, 0, 2);
    initial_frame->function_name = ENTRY_FUNCTION_NAME;

    Vector* cmdline = new Vector();
    auto limit = commandline_arguments.size();
    for (decltype(limit) i = 0; i < limit; ++i) {
        cmdline->push(new String(commandline_arguments[i]));
    }
    initial_frame->regset->set(1, cmdline);

    Process* t = new Process(initial_frame, this, jump_base, nullptr);
    t->detach();
    t->priority(16);
    t->begin();

    procs->push_back(t);
}


viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(CPU *acpu, decltype(procs) ps):
    attached_cpu(acpu),
    current_process_index(0),
    procs(ps)
{
}
