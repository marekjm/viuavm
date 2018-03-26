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

#ifndef VIUA_CPU_H
#define VIUA_CPU_H

#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/include/module.h>
#include <viua/process.h>
#include <viua/types/prototype.h>


namespace viua {
namespace process {
class Process;
}
namespace scheduler {
class VirtualProcessScheduler;

namespace ffi {
class ForeignFunctionCallRequest;
}
}  // namespace scheduler
}  // namespace viua


namespace viua { namespace kernel {
class Mailbox {
    mutable std::mutex mailbox_mutex;
    std::vector<std::unique_ptr<viua::types::Value>> messages;

  public:
    auto send(std::unique_ptr<viua::types::Value>) -> void;
    auto receive(std::queue<std::unique_ptr<viua::types::Value>>&) -> void;
    auto size() const -> decltype(messages)::size_type;

    Mailbox() = default;
    Mailbox(Mailbox&&);
};

class ProcessResult {
    mutable std::mutex result_mutex;

    /*
     * A process that has finished may either return a value, or
     * throw an exception.
     * Slots for both of these cases must be provided.
     */
    std::unique_ptr<viua::types::Value> value_returned;
    std::unique_ptr<viua::types::Value> exception_thrown;

    /*
     * A flag set to 'true' once the process has finished execution.
     */
    std::atomic_bool done;

  public:
    /*
     * Check if the process has stopped (for any reason).
     */
    auto stopped() const -> bool;

    /*
     * Check if the process has been terminated.
     */
    auto terminated() const -> bool;

    /*
     * Resolve a process to some return value.
     * This means that the execution succeeded.
     */
    auto resolve(std::unique_ptr<viua::types::Value>) -> void;

    /*
     * Raise an exception that killed the process.
     * This means that the execution failed.
     */
    auto raise(std::unique_ptr<viua::types::Value>) -> void;

    /*
     * Transfer return value and exception from process result.
     */
    auto transfer_exception() -> std::unique_ptr<viua::types::Value>;
    auto transfer_result() -> std::unique_ptr<viua::types::Value>;

    ProcessResult() = default;
    ProcessResult(ProcessResult&&);
};

class Kernel {
#ifdef AS_DEBUG_HEADER
  public:
#endif
    /*  Bytecode pointer is a pointer to program's code.
     *  Size and executable offset are metadata exported from bytecode dump.
     */
    std::unique_ptr<viua::internals::types::byte[]> bytecode;
    viua::internals::types::bytecode_size bytecode_size;
    viua::internals::types::bytecode_size executable_offset;

    // Map of the typesystem currently existing inside the VM.
    std::map<std::string, std::unique_ptr<viua::types::Prototype>> typesystem;

    /*  Function and block names mapped to bytecode addresses.
     */
    std::map<std::string, viua::internals::types::bytecode_size>
        function_addresses;
    std::map<std::string, viua::internals::types::bytecode_size>
        block_addresses;

    std::map<std::string, std::pair<std::string, viua::internals::types::byte*>>
        linked_functions;
    std::map<std::string, std::pair<std::string, viua::internals::types::byte*>>
        linked_blocks;
    std::map<std::string,
             std::pair<viua::internals::types::bytecode_size,
                       std::unique_ptr<viua::internals::types::byte[]>>>
        linked_modules;

    int return_code;

    /*
     *  VIRTUAL PROCESSES SCHEDULING
     *
     *  List of virtual processes that do not belong to any scheduler, and
     *  are waiting to be adopted, along with means of synchronization of
     *  concurrent accesses to said list.
     *  Schedulers can post their spawned processes to the Kernel to let
     *  other schedulers execute them (sometimes, a scheduler may fetch
     *  its own process back).
     *
     *  Also, a list of spawned VP schedulers.
     */
    // list of virtual processes not associated with any VP scheduler
    std::vector<std::unique_ptr<viua::process::Process>> free_virtual_processes;
    std::mutex free_virtual_processes_mutex;
    std::condition_variable free_virtual_processes_cv;
    // list of running VP schedulers, pairs of {scheduler-pointer, thread}
    std::vector<
        std::pair<viua::scheduler::VirtualProcessScheduler*, std::thread>>
        virtual_process_schedulers;
    // list of idle VP schedulers
    std::vector<viua::scheduler::VirtualProcessScheduler*>
        idle_virtual_process_schedulers;

    std::atomic<viua::internals::types::processes_count> running_processes{0};

    static const viua::internals::types::schedulers_count
        default_vp_schedulers_limit = 2;
    viua::internals::types::schedulers_count vp_schedulers_limit;

    /*  This is the interface between programs compiled to VM bytecode and
     *  extension libraries written in C++.
     */
    std::map<std::string, ForeignFunction*> foreign_functions;
    std::mutex foreign_functions_mutex;

    /** This is the mapping Viua uses to dispatch methods on pure-C++ classes.
     */
    std::map<std::string, ForeignMethod> foreign_methods;

    // Foreign function call requests are placed here to be executed later.
    std::vector<
        std::unique_ptr<viua::scheduler::ffi::ForeignFunctionCallRequest>>
        foreign_call_queue;
    std::mutex foreign_call_queue_mutex;
    std::condition_variable foreign_call_queue_condition;
    static const viua::internals::types::schedulers_count
        default_ffi_schedulers_limit = 2;
    viua::internals::types::schedulers_count ffi_schedulers_limit;
    std::vector<std::unique_ptr<std::thread>> foreign_call_workers;

    std::vector<void*> cxx_dynamic_lib_handles;

    std::map<viua::process::PID, Mailbox> mailboxes;
    std::mutex mailbox_mutex;

    /*
     * Only processes that were not disowned have an entry here.
     * Result of a process may be fetched only once - it is then deleted.
     * It must also be deleted when no process would be able to fetch it to
     * prevent return value leaks.
     */
    std::map<viua::process::PID, ProcessResult> process_results;
    mutable std::mutex process_results_mutex;

  public:
    /*  Methods dealing with dynamic library loading.
     */
    void load_module(std::string);
    void load_native_library(const std::string&);
    void load_foreign_library(const std::string&);

    // debug and error reporting flags
    bool debug, errors;

    std::vector<std::string> commandline_arguments;

    /*  Public API of the Kernel provides basic actions:
     *
     *      * load bytecode,
     *      * set its size,
     *      * tell the Kernel where to start execution,
     *      * kick the Kernel so it starts running,
     */
    Kernel& load(std::unique_ptr<viua::internals::types::byte[]>);
    Kernel& bytes(viua::internals::types::bytecode_size);

    Kernel& mapfunction(const std::string&,
                        viua::internals::types::bytecode_size);
    Kernel& mapblock(const std::string&, viua::internals::types::bytecode_size);

    Kernel& register_external_function(const std::string&, ForeignFunction*);
    Kernel& remove_external_function(std::string);

    /*  Methods dealing with typesystem related tasks.
     */
    bool is_class(const std::string&) const;
    bool class_accepts(const std::string&, const std::string&) const;
    std::vector<std::string> inheritance_chain_of(const std::string&) const;
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

    void register_prototype(const std::string&,
                            std::unique_ptr<viua::types::Prototype>);
    void register_prototype(std::unique_ptr<viua::types::Prototype>);

    /// These two methods are used to inject pure-C++ classes into machine's
    /// typesystem.
    Kernel& register_foreign_prototype(const std::string&,
                                       std::unique_ptr<viua::types::Prototype>);
    Kernel& register_foreign_method(const std::string&, ForeignMethod);

    void request_foreign_function_call(Frame*, viua::process::Process*);
    void request_foreign_method_call(const std::string&,
                                     viua::types::Value*,
                                     Frame*,
                                     viua::kernel::RegisterSet*,
                                     viua::kernel::RegisterSet*,
                                     viua::process::Process*);

    void post_free_process(std::unique_ptr<viua::process::Process>);

    auto create_mailbox(const viua::process::PID)
        -> viua::internals::types::processes_count;
    auto delete_mailbox(const viua::process::PID)
        -> viua::internals::types::processes_count;

    auto create_result_slot_for(viua::process::PID) -> void;
    auto detach_process(const viua::process::PID) -> void;
    auto record_process_result(viua::process::Process*) -> void;
    auto is_process_joinable(const viua::process::PID) const -> bool;
    auto is_process_stopped(const viua::process::PID) const -> bool;
    auto is_process_terminated(const viua::process::PID) const -> bool;
    auto transfer_exception_of(const viua::process::PID)
        -> std::unique_ptr<viua::types::Value>;
    auto transfer_result_of(const viua::process::PID)
        -> std::unique_ptr<viua::types::Value>;

    void send(const viua::process::PID, std::unique_ptr<viua::types::Value>);
    void receive(const viua::process::PID,
                 std::queue<std::unique_ptr<viua::types::Value>>&);
    uint64_t pids() const;

    auto static no_of_vp_schedulers()
        -> viua::internals::types::schedulers_count;
    auto static no_of_ffi_schedulers()
        -> viua::internals::types::schedulers_count;
    auto static is_tracing_enabled() -> bool;

    int run();

    int exit() const;

    Kernel();
    ~Kernel();
};
}}  // namespace viua::kernel

#endif
