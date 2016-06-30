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

            public:

            bool executeQuant(Process*, unsigned);

            Process* bootstrap(const std::vector<std::string>&, byte*);

            VirtualProcessScheduler(CPU *attached_cpu);
        };
    }
}


#endif
