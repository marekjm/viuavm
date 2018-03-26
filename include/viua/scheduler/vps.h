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

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/frame.h>


namespace viua {
namespace process {
class Process;
}
namespace kernel {
class Kernel;
}
}  // namespace viua


namespace viua {
namespace scheduler {
class VirtualProcessScheduler {
    /** Scheduler of Viua VM virtual processes.
     */
    viua::kernel::Kernel* attached_kernel;

    /*
     * Variables set below control whether the VM should gather and
     * emit additional (debugging, profiling, tracing) information
     * regarding executed code.
     */
    const bool tracing_enabled;

    std::vector<std::unique_ptr<viua::process::Process>>* free_processes;
    std::mutex* free_processes_mutex;
    std::condition_variable* free_processes_cv;

    viua::process::Process* main_process;
    std::vector<std::unique_ptr<viua::process::Process>> processes;
    decltype(processes)::size_type current_process_index;

    int exit_code;

    // if scheduler hits heavy load it starts posting processes to
    // viua::kernel::Kernel to let other schedulers at them
    decltype(processes)::size_type current_load;
    std::atomic_bool shut_down;
    std::thread scheduler_thread;

  public:
    viua::kernel::Kernel* kernel() const;

    bool is_class(const std::string&) const;
    bool class_accepts(const std::string&, const std::string&) const;
    auto inheritance_chain_of(const std::string& name) const
        -> decltype(attached_kernel->inheritance_chain_of(name));
    bool is_local_function(const std::string&) const;
    bool is_linked_function(const std::string&) const;
    bool is_native_function(const std::string&) const;
    bool is_foreign_method(const std::string&) const;
    bool is_foreign_function(const std::string&) const;

    bool is_block(const std::string&) const;
    bool is_local_block(const std::string&) const;
    bool is_linked_block(const std::string&) const;
    std::pair<viua::internals::types::byte*, viua::internals::types::byte*>
    get_entry_point_of_block(const std::string&) const;

    std::string resolve_method_name(const std::string&,
                                    const std::string&) const;
    std::pair<viua::internals::types::byte*, viua::internals::types::byte*>
    get_entry_point_of(const std::string&) const;

    void register_prototype(std::unique_ptr<viua::types::Prototype>);

    void request_foreign_function_call(Frame*, viua::process::Process*) const;
    void request_foreign_method_call(const std::string&, viua::types::Value*,
                                     Frame*, viua::kernel::RegisterSet*,
                                     viua::kernel::RegisterSet*,
                                     viua::process::Process*);

    void load_module(std::string);

    auto cpi() const -> decltype(processes)::size_type;
    auto size() const -> decltype(processes)::size_type;

    viua::process::Process* process(decltype(processes)::size_type);
    viua::process::Process* process();
    viua::process::Process* spawn(std::unique_ptr<Frame>,
                                  viua::process::Process*, bool);

    void send(const viua::process::PID, std::unique_ptr<viua::types::Value>);
    void receive(const viua::process::PID,
                 std::queue<std::unique_ptr<viua::types::Value>>&);

    auto is_joinable(const viua::process::PID) const -> bool;
    auto is_stopped(const viua::process::PID) const -> bool;
    auto is_terminated(const viua::process::PID) const -> bool;
    auto transfer_exception_of(const viua::process::PID) const
        -> std::unique_ptr<viua::types::Value>;
    auto transfer_result_of(const viua::process::PID) const
        -> std::unique_ptr<viua::types::Value>;

    bool execute_quant(viua::process::Process*,
                       viua::internals::types::process_time_slice_type);
    bool burst();

    void operator()();

    void bootstrap(const std::vector<std::string>&);
    void launch();
    void shutdown();
    void join();
    int exit() const;

    VirtualProcessScheduler(
        viua::kernel::Kernel*,
        std::vector<std::unique_ptr<viua::process::Process>>*, std::mutex*,
        std::condition_variable*, const bool = false);
    VirtualProcessScheduler(VirtualProcessScheduler&&);
    ~VirtualProcessScheduler();
};
}  // namespace scheduler
}  // namespace viua


#endif
