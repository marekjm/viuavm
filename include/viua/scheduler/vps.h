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

#ifndef VIUA_SCHEDULER_VPS_H
#define VIUA_SCHEDULER_VPS_H

#include <vector>
#include <queue>
#include <string>
#include <utility>
#include <memory>
#include <mutex>
#include <atomic>
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

            std::vector<std::unique_ptr<Process>> *free_processes;
            std::mutex *free_processes_mutex;
            std::condition_variable *free_processes_cv;

            Process *main_process;
            std::vector<std::unique_ptr<Process>> processes;
            decltype(processes)::size_type current_process_index;

            std::string watchdog_function;
            std::unique_ptr<Process> watchdog_process;

            int exit_code;

            void resurrectWatchdog();

            // if scheduler hits heavy load it starts posting processes to CPU
            // to let other schedulers at them
            const decltype(processes)::size_type heavy_load = 16;
            const decltype(processes)::size_type light_load = 4;
            std::atomic_bool shut_down;

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
            auto size() const -> decltype(processes)::size_type;

            Process* process(decltype(processes)::size_type);
            Process* process();
            Process* spawn(std::unique_ptr<Frame>, Process*);
            void spawnWatchdog(std::unique_ptr<Frame>);

            void receive(const PID, std::queue<std::unique_ptr<Type>>&);

            bool executeQuant(Process*, unsigned);
            bool burst();

            void operator()();

            void bootstrap(const std::vector<std::string>&);
            void shutdown();
            int exit() const;

            VirtualProcessScheduler(CPU*, std::vector<std::unique_ptr<Process>>*, std::mutex*, std::condition_variable*);
            ~VirtualProcessScheduler();
        };
    }
}


#endif
