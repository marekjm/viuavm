#ifndef VIUA_SCHEDULER_VPS_H
#define VIUA_SCHEDULER_VPS_H

#include <vector>
#include <string>
#include <utility>
#include <viua/bytecode/bytetypedef.h>


class CPU;
class Process;


namespace viua {
    namespace scheduler {
        class VirtualProcessScheduler {
            /** Scheduler of Viua VM virtual processes.
             */
            CPU *attached_cpu;

            std::vector<Process*> processes;
            decltype(processes)::size_type current_process_index;

            // this will be used during a transition period
            // it is a pointer to processes vector inside the CPU
            decltype(processes) *procs;

            public:

            CPU* cpu() const;

            bool isClass(const std::string&) const;
            auto inheritanceChainOf(const std::string& name) const -> decltype(attached_cpu->inheritanceChainOf(name));
            std::pair<byte*, byte*> getEntryPointOf(const std::string&) const;

            auto cpi() const -> decltype(processes)::size_type;
            Process* process(decltype(processes)::size_type);
            Process* process();

            bool executeQuant(Process*, unsigned);
            bool burst();

            void bootstrap(const std::vector<std::string>&, byte*);

            VirtualProcessScheduler(CPU*, decltype(procs) ps);
        };
    }
}


#endif
