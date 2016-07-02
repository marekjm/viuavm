#ifndef VIUA_SCHEDULER_VPS_H
#define VIUA_SCHEDULER_VPS_H

#include <vector>
#include <string>


class CPU;
class Process;


namespace viua {
    namespace scheduler {
        class VirtualProcessScheduler {
            /** Scheduler of Viua VM virtual processes.
             */
            CPU *cpu;

            std::vector<Process*> processes;
            decltype(processes)::size_type current_process_index;

            // this will be used during a transition period
            // it is a pointer to processes vector inside the CPU
            decltype(processes) *procs;

            public:

            bool executeQuant(Process*, unsigned);
            bool burst();

            Process* bootstrap(const std::vector<std::string>&, byte*);

            VirtualProcessScheduler(CPU *attached_cpu, decltype(procs) ps);
        };
    }
}


#endif
