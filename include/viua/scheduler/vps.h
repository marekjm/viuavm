#ifndef VIUA_SCHEDULER_VPS_H
#define VIUA_SCHEDULER_VPS_H


class CPU;


namespace viua {
    namespace scheduler {
        class VirtualProcessScheduler {
            /** Scheduler of Viua VM virtual processes.
             */
            CPU *cpu;

            public:

            VirtualProcessScheduler(CPU *attached_cpu);
        };
    }
}


#endif
