#ifndef VIUA_SCHEDULER_VPS_H
#define VIUA_SCHEDULER_VPS_H

#include <vector>
#include <string>
#include <utility>
#include <viua/bytecode/bytetypedef.h>
#include <viua/cpu/frame.h>


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
            bool classAccepts(const std::string&, const std::string&) const;
            auto inheritanceChainOf(const std::string& name) const -> decltype(attached_cpu->inheritanceChainOf(name));
            bool isLocalFunction(const std::string&) const;
            bool isLinkedFunction(const std::string&) const;
            bool isNativeFunction(const std::string&) const;
            bool isForeignMethod(const std::string&) const;
            bool isForeignFunction(const std::string&) const;

            bool isBlock(const std::string&) const;
            bool isLocalBlock(const std::string&) const;
            bool isLinkedBlock(const std::string&) const;
            std::pair<byte*, byte*> getEntryPointOfBlock(const std::string&) const;

            std::string resolveMethodName(const std::string&, const std::string&) const;
            std::pair<byte*, byte*> getEntryPointOf(const std::string&) const;

            void registerPrototype(Prototype*);

            void requestForeignFunctionCall(Frame*, Process*) const;
            void requestForeignMethodCall(const std::string&, Type*, Frame*, RegisterSet*, RegisterSet*, Process*);

            void loadNativeLibrary(const std::string&);
            void loadForeignLibrary(const std::string&);

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
