/*
 *  Copyright (C) 2015-2019 Marek Marecki
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

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/include/module.h>
#include <viua/process.h>


namespace viua {
namespace process {
class Process;
}
namespace scheduler {
class Process_scheduler;

namespace ffi {
class Foreign_function_call_request;
}

namespace io {
class IO_request;
struct IO_interaction;
}  // namespace io
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

class Process_result {
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

    Process_result() = default;
    Process_result(Process_result&&);
};

class Kernel {
#ifdef VIUAVM_AS_DEBUG_HEADER
  public:
#endif
    /*
     * Bytecode pointer is a pointer to program's code. Size and executable
     * offset are metadata exported from bytecode dump.
     */
    std::unique_ptr<uint8_t[]> bytecode;
    viua::bytecode::codec::bytecode_size_type bytecode_size;
    viua::bytecode::codec::bytecode_size_type executable_offset;

    /*  Function and block names mapped to bytecode addresses.
     */
    std::map<std::string, viua::bytecode::codec::bytecode_size_type>
        function_addresses;
    std::map<std::string, viua::bytecode::codec::bytecode_size_type>
        block_addresses;

    std::map<std::string, std::pair<std::string, uint8_t*>>
        linked_functions;
    std::map<std::string, std::pair<std::string, uint8_t*>>
        linked_blocks;
    std::map<std::string,
             std::pair<viua::bytecode::codec::bytecode_size_type,
                       std::unique_ptr<uint8_t[]>>>
        linked_modules;

    int return_code {-1};

    /*
     * The number of running processes. This is needed to calculate the load on
     * schedulers and other things (e.g. if there are no processes we can shut
     * down).
     */
    std::atomic<size_t> running_processes{0};

    /*
     * VIRTUAL PROCESS SCHEDULING
     */
    std::vector<std::unique_ptr<viua::scheduler::Process_scheduler>>
        process_schedulers;

    /*
     * This condition variable is used to signal that there is a process
     * available to be stolen. A signal for work stealing algorithm to kick in
     * and do its job.
     */
    std::condition_variable process_spawned_cv;
    std::mutex process_spawned_mtx;
    // FIXME Maybe use a ring buffer instead of a deque to make the size of
    // the "who spawned a process" list bounded? If all schedulers are populated
    // and don't need to steal work this list could otherwise get uncomfortably
    // long really quick.
    std::deque<viua::scheduler::Process_scheduler*> process_spawned_by;

    /*
     * FFI MODULES
     *
     * This is the interface between programs compiled to VM bytecode and
     * extension libraries written in C++. Modules written in C++ are opened as
     * shared objects, the functions they export are loaded and made available
     * to running code.
     */
    std::map<std::string, ForeignFunction*> foreign_functions;
    std::mutex foreign_functions_mutex;
    std::vector<void*> cxx_dynamic_lib_handles;

    /*
     * FFI SCHEDULING
     *
     */
    // Foreign function call requests are placed here to be executed later.
    std::vector<
        std::unique_ptr<viua::scheduler::ffi::Foreign_function_call_request>>
        foreign_call_queue;
    std::mutex foreign_call_queue_mutex;
    std::condition_variable foreign_call_queue_condition;
    std::vector<std::unique_ptr<std::thread>> foreign_call_workers;

    /*
     * I/O SCHEDULING
     */
    std::deque<std::unique_ptr<viua::scheduler::io::IO_interaction>>
        io_request_queue;
    std::mutex io_request_mutex;
    std::condition_variable io_request_cv;
    std::vector<std::unique_ptr<std::thread>> io_workers;

  public:
    class IO_result {
        IO_result(bool const, std::unique_ptr<viua::types::Value>);

      public:
        std::unique_ptr<viua::types::Value> value;
        std::unique_ptr<viua::types::Value> error;

        bool is_complete;
        bool is_cancelled;
        bool is_successful;

        IO_result()                 = default;
        IO_result(IO_result const&) = delete;
        auto operator=(IO_result const&) -> IO_result& = delete;
        IO_result(IO_result&& that)
                : value{std::move(that.value)}
                , error{std::move(that.error)}
                , is_complete{that.is_complete}
                , is_cancelled{that.is_cancelled}
                , is_successful{that.is_successful}
        {}
        auto operator=(IO_result&& that) -> IO_result& = delete;

        static auto make_success(std::unique_ptr<viua::types::Value>)
            -> IO_result;
        static auto make_error(std::unique_ptr<viua::types::Value>)
            -> IO_result;
    };

  private:
    // FIXME Use viua::scheduler::io::IO_interaction::id_type.
    mutable std::mutex io_requests_mtx;
    std::map<std::tuple<uint64_t, uint64_t>,
             viua::scheduler::io::IO_interaction*>
        io_requests;
    mutable std::mutex io_result_mtx;
    std::map<std::tuple<uint64_t, uint64_t>, IO_result> io_results;

    /*
     * MESSAGE PASSING
     *
     * Why are mailboxes are kept inside the kernel? To remove the need to look
     * for the process on possibly many schedulers (as we would need to lock 'em
     * all to avoid race conditions with process migration algorithms), and to
     * make it easier to send and receive messages (just call kernel's routines
     * instead of finding the right scheduler and process). This adds a level of
     * indirection but in this case I think it is justified.
     */
    std::map<viua::process::PID, Mailbox> mailboxes;
    std::mutex mailbox_mutex;

    /*
     * Only processes that were not disowned have an entry here.
     * Result of a process may be fetched only once - it is then deleted.
     * It must also be deleted when no process would be able to fetch it to
     * prevent return value leaks.
     */
    std::map<viua::process::PID, Process_result> process_results;
    mutable std::mutex process_results_mutex;

  public:
    /*  Methods dealing with dynamic library loading.
     */
    void load_module(std::string);
    void load_native_module(std::string_view const, std::string const&);
    void load_bytecode_module(std::string_view const, std::string const&);

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
    Kernel& load(std::unique_ptr<uint8_t[]>);
    Kernel& bytes(viua::bytecode::codec::bytecode_size_type);

    Kernel& mapfunction(std::string const&,
                        viua::bytecode::codec::bytecode_size_type);
    Kernel& mapblock(std::string const&, viua::bytecode::codec::bytecode_size_type);

    Kernel& register_external_function(std::string const&, ForeignFunction*);
    Kernel& remove_external_function(std::string);

    bool is_local_function(std::string const&) const;
    bool is_linked_function(std::string const&) const;
    bool is_native_function(std::string const&) const;
    bool is_foreign_function(std::string const&) const;

    bool is_block(std::string const&) const;
    bool is_local_block(std::string const&) const;
    bool is_linked_block(std::string const&) const;
    auto get_entry_point_of_block(std::string const&) const
        -> std::pair<viua::internals::types::Op_address_type,
                     viua::internals::types::Op_address_type>;

    auto get_entry_point_of(std::string const&) const
        -> std::pair<viua::internals::types::Op_address_type,
                     viua::internals::types::Op_address_type>;
    auto module_at(uint8_t const* const) const -> std::optional<std::string>;
    auto in_which_function(std::string const, uint64_t const) const -> std::optional<std::string>;

    void request_foreign_function_call(std::unique_ptr<Frame>,
                                       viua::process::Process&);

    auto steal_processes()
        -> std::vector<std::unique_ptr<viua::process::Process>>;
    auto notify_about_process_spawned(viua::scheduler::Process_scheduler*)
        -> void;
    auto notify_about_process_death() -> void;
    auto process_count() const -> size_t;

    auto create_mailbox(const viua::process::PID)
        -> size_t;
    auto delete_mailbox(const viua::process::PID)
        -> size_t;

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

    auto schedule_io(std::unique_ptr<viua::scheduler::io::IO_interaction>)
        -> void;
    auto cancel_io(std::tuple<uint64_t, uint64_t> const) -> void;
    auto complete_io(std::tuple<uint64_t, uint64_t> const, IO_result) -> void;
    auto io_complete(std::tuple<uint64_t, uint64_t> const) const -> bool;
    auto io_result(std::tuple<uint64_t, uint64_t> const)
        -> std::unique_ptr<viua::types::Value>;

    auto static no_of_process_schedulers()
        -> size_t;
    auto static no_of_ffi_schedulers()
        -> size_t;
    auto static no_of_io_schedulers()
        -> size_t;
    auto static is_tracing_enabled() -> bool;

    int run();

    int exit() const;

    Kernel();
    ~Kernel();
};
}}  // namespace viua::kernel

#endif
