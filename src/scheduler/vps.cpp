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

bool viua::scheduler::VirtualProcessScheduler::burst(vector<Process*>& processes_list) {
    if (not processes_list.size()) {
        // make CPU stop if there are no processes_list to run
        return false;
    }

    bool ticked = false;

    vector<Process*> running_processes_list;
    decltype(running_processes_list) dead_processes_list;
    bool abort_because_of_process_termination = false;
    for (decltype(running_processes_list)::size_type i = 0; i < processes_list.size(); ++i) {
        auto th = processes_list[i];

        ticked = (executeQuant(th, th->priority()) or ticked);

        if (th->terminated() and not th->joinable() and th->parent() == nullptr) {
            if (cpu->currentWatchdog() == nullptr) {
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
                cpu->currentWatchdog()->pass(death_message);

                // push broken process to dead processes_list list to
                // erase it later
                dead_processes_list.push_back(th);

                // copy what is left after this broken process to running processes_list array to
                // prevent losing them
                // because after the loop the `processes_list` vector is reset
                for (decltype(running_processes_list)::size_type j = i+1; j < processes_list.size(); ++j) {
                    running_processes_list.push_back(processes_list[j]);
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
        processes_list.erase(processes_list.begin(), processes_list.end());
        for (decltype(running_processes_list)::size_type i = 0; i < running_processes_list.size(); ++i) {
            processes_list.push_back(running_processes_list[i]);
        }
    }

    Process *watchdog_process = cpu->currentWatchdog();
    while (watchdog_process != nullptr and not watchdog_process->suspended()) {
        executeQuant(watchdog_process, 0);
        watchdog_process = cpu->currentWatchdog();
        if (watchdog_process->terminated() or watchdog_process->stopped()) {
            cpu->resurrectWatchdog();
        }
    }

    return ticked;
}

Process* viua::scheduler::VirtualProcessScheduler::bootstrap(const vector<string>& commandline_arguments, byte *jump_base) {
    Frame *initial_frame = new Frame(nullptr, 0, 2);
    initial_frame->function_name = ENTRY_FUNCTION_NAME;

    Vector* cmdline = new Vector();
    auto limit = commandline_arguments.size();
    for (decltype(limit) i = 0; i < limit; ++i) {
        cmdline->push(new String(commandline_arguments[i]));
    }
    initial_frame->regset->set(1, cmdline);

    Process* t = new Process(initial_frame, cpu, jump_base, nullptr);
    t->detach();
    t->priority(16);
    t->begin();

    return t;
}


viua::scheduler::VirtualProcessScheduler::VirtualProcessScheduler(CPU *attached_cpu):
    cpu(attached_cpu),
    current_process_index(0)
{
}
