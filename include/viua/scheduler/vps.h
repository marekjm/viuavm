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
#include <thread>
#include <mutex>
#include <atomic>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/frame.h>


namespace viua {
    namespace process {
        class Process;
    }
    namespace kernel {
        class Kernel;
    }
}


namespace viua {
    namespace scheduler {
        class VirtualProcessScheduler {
            /** Scheduler of Viua VM virtual processes.
             */
            viua::kernel::Kernel *attached_kernel;

            std::vector<std::unique_ptr<viua::process::Process>> *free_processes;
            std::mutex *free_processes_mutex;
            std::condition_variable *free_processes_cv;

            viua::process::Process *main_process;
            std::vector<std::unique_ptr<viua::process::Process>> processes;
            decltype(processes)::size_type current_process_index;

            int exit_code;

            // if scheduler hits heavy load it starts posting processes to viua::kernel::Kernel
            // to let other schedulers at them
            decltype(processes)::size_type current_load;
            std::atomic_bool shut_down;
            std::thread scheduler_thread;

            public:

            viua::kernel::Kernel* kernel() const;

            bool isClass(const std::string&) const;
            bool classAccepts(const std::string&, const std::string&) const;
            auto inheritanceChainOf(const std::string& name) const -> decltype(attached_kernel->inheritanceChainOf(name));
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

            void registerPrototype(viua::types::Prototype*);

            void requestForeignFunctionCall(Frame*, viua::process::Process*) const;
            void requestForeignMethodCall(const std::string&, viua::types::Type*, Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*);

            void loadNativeLibrary(const std::string&);
            void loadForeignLibrary(const std::string&);

            auto cpi() const -> decltype(processes)::size_type;
            auto size() const -> decltype(processes)::size_type;

            viua::process::Process* process(decltype(processes)::size_type);
            viua::process::Process* process();
            viua::process::Process* spawn(std::unique_ptr<Frame>, viua::process::Process*, bool);

            void send(const viua::process::PID, std::unique_ptr<viua::types::Type>);
            void receive(const viua::process::PID, std::queue<std::unique_ptr<viua::types::Type>>&);

            bool executeQuant(viua::process::Process*, unsigned);
            bool burst();

            void operator()();

            void bootstrap(const std::vector<std::string>&);
            void launch();
            void shutdown();
            void join();
            int exit() const;

            VirtualProcessScheduler(viua::kernel::Kernel*, std::vector<std::unique_ptr<viua::process::Process>>*, std::mutex*, std::condition_variable*);
            VirtualProcessScheduler(VirtualProcessScheduler&&);
            ~VirtualProcessScheduler();
        };
    }
}


#endif
