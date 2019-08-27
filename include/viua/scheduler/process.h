/*
 *  Copyright (C) 2019 Marek Marecki
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

#ifndef VIUA_SCHEDULER_PROCESS_H
#define VIUA_SCHEDULER_PROCESS_H

#include <atomic>
#include <deque>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>
#include <viua/kernel/frame.h>
#include <viua/pid.h>

namespace viua {
    namespace process {
        class Process;
    }
    namespace kernel {
        class Kernel;
    }
    namespace types {
        class IO_interaction;
    }
}

namespace viua { namespace scheduler {
class Process_scheduler {
  public:
    using process_type = viua::process::Process;
    using process_queue_type = std::deque<std::unique_ptr<process_type>>;
    using size_type = process_queue_type::size_type;
    using id_type = size_t;

  private:
    /*
     * The ID assigned to this scheduler by the kernel. Does not do much and is
     * used mostly to identify the scheduler in logs.
     */
    id_type const assigned_id;

    /*
     * The kernel to which the scheduler is attached. There is only one kernel
     * present in the VM (while there are possibly many process schedulers) but
     * it is better to have the pointer be a scheduler's local variable than a
     * global one.
     */
    viua::kernel::Kernel& attached_kernel;

    /*
     * Main process of a scheduler.
     */
    process_type* main_process = nullptr;

    /*
     * The set of processes running on this scheduler. It may be modified either
     * when a new process is spawned, or when the work stealing algorithm kicks
     * in (two schedulers' process queues are modified then), or when the load
     * balancing algorithm kicks in (possibly many schedulers are affected when
     * this happens).
     */
    process_queue_type process_queue;
    mutable std::mutex process_queue_mtx;

    auto push(std::unique_ptr<process_type>) -> void;
    auto pop() -> std::unique_ptr<process_type>;
    auto size() const -> size_type;
    auto empty() const -> bool;

    /*
     * Exit code of the scheduler. It is useful only for the scheduler running
     * the main function of launched program.
     */
    std::optional<int> exit_code;

    /*
     * Variable signifying that the scheduler should shut down at the earliest
     * possible moment. Without this set to true the scheduler will wait until
     * a new process is spawned and then try to steal it instead of just
     * shutting down.
     */
    std::atomic_bool should_shut_down;

    /*
     * The thread that runs the scheduler's code.
     */
    std::thread scheduler_thread;
    auto operator()() -> void;

  public:
    Process_scheduler(viua::kernel::Kernel&, id_type const);
    Process_scheduler(Process_scheduler const&) = delete;
    auto operator=(Process_scheduler const&) -> Process_scheduler& = delete;
    Process_scheduler(Process_scheduler&&) = delete;
    auto operator=(Process_scheduler&&) -> Process_scheduler& = delete;
    ~Process_scheduler();

    auto id() const -> id_type;

    /*
     * Bootstrap the scheduler by creating a call to program entry function.
     * The entry function has the same role (setting up the environment and
     * calling user-supplied main) as the `_start` function in C (as emitted by
     * GCC, at least).
     */
    auto bootstrap(std::vector<std::string>) -> void;

    /*
     * Spawn a new process. This will create and start a new process on this
     * scheduler, create a mailbox inside the VM kernel and perform some other
     * bookkeeping that is needed for correct operation of the VM.
     */
    auto spawn(std::unique_ptr<Frame>, process_type*, bool) -> process_type*;
    auto give_up_processes() -> std::vector<std::unique_ptr<process_type>>;

    /*
     * Process state inquiry functions.
     */
    auto is_joinable(viua::process::PID const) const -> bool;
    auto is_stopped(viua::process::PID const) const -> bool;
    auto is_terminated(viua::process::PID const) const -> bool;

    /*
     * Transfer the exception that killed a process to the current process, or
     * the return value of the process (if it has successfully terminated).
     */
    auto transfer_exception_of(viua::process::PID const) const -> std::unique_ptr<viua::types::Value>;
    auto transfer_result_of(viua::process::PID const) const -> std::unique_ptr<viua::types::Value>;

    /*
     * This is the message exchange interface. It talks to the kernel to push
     * messages to and pop them from kernel-held queues.
     */
    auto send(viua::process::PID const, std::unique_ptr<viua::types::Value>) -> void;
    auto receive(viua::process::PID const,
                 std::queue<std::unique_ptr<viua::types::Value>>&) -> void;

    /*
     * FFI gateway for Viua processes. It initiates a call on a FFI scheduler
     * and needs to talk to the kernel to do this - as the process scheduler
     * does not hold any references to the FFI schedulers list.
     */
    auto request_ffi_call(std::unique_ptr<Frame>, viua::process::Process&) -> void;

    /*
     * Function type inquiry. This is used when spawning processes and calling
     * functions as only Viua-bytecode functions can be made into processes, and
     * foreign functions must be run on a different type of scheduler.
     */
    auto is_native_function(std::string const) const -> bool;
    auto is_foreign_function(std::string const) const -> bool;
    auto is_block(std::string const) const -> bool;

    /*
     * Interface for module loading and reloading.
     */
    auto load_module(std::string const) -> void;

    /*
     * Functions providing access to entry point and bytecode base information.
     * This is needed to properly set offset bases for bytecode modules.
     */
    auto get_entry_point_of_block(std::string const) const
        -> std::pair<viua::internals::types::Op_address_type,
                     viua::internals::types::Op_address_type>;
    auto get_entry_point_of_function(std::string const&) const
        -> std::pair<viua::internals::types::Op_address_type,
                     viua::internals::types::Op_address_type>;

    /*
     * Scheduler management interface. Launching, stopping, etc. related to the
     * scheduler itself.
     */
    auto launch() -> void;
    auto shutdown() -> void;
    auto join() -> void;
    auto exit() const -> int;

    auto kernel() const -> viua::kernel::Kernel& {
        return attached_kernel;
    }

    auto schedule_io(std::unique_ptr<viua::types::IO_interaction>) -> void;
    auto cancel_io(std::tuple<uint64_t, uint64_t> const) -> void;
    auto io_complete(std::tuple<uint64_t, uint64_t> const) const -> bool;
    auto io_result(std::tuple<uint64_t, uint64_t> const) -> std::unique_ptr<viua::types::Value>;
};
}}

#endif
