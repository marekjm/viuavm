/*
 *  Copyright (C) 2015-2020 Marek Marecki
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

#ifndef VIUA_PROCESS_H
#define VIUA_PROCESS_H

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/codec/main.h>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/tryframe.h>
#include <viua/pid.h>
#include <viua/scheduler/io/interactions.h>
#include <viua/types/exception.h>
#include <viua/types/value.h>


class Halt_exception : public std::runtime_error {
  public:
    Halt_exception() : std::runtime_error("execution halted") {}
};


namespace viua::types {
struct IO_interaction;
}


namespace viua { namespace scheduler {
class Process_scheduler;
}}  // namespace viua::scheduler

namespace viua { namespace process {
class Process;

using viua::internals::types::Op_address_type;

class Stack {
  public:
    enum class STATE {
        /*
         * Before stack begins executing.
         */
        UNINITIALISED,

        /*
         * Normal state.
         * Stack is executing instructions normally.
         */
        RUNNING,

        /*
         * Stack is suspended because of deferred calls it triggered when
         * returning from a call normally. The VM should finish executing any
         * stacks spawned by this stack's deferred calls, and then return to
         * continue executing this stack.
         */
        SUSPENDED_BY_DEFERRED_ON_FRAME_POP,

        /*
         * Stack is suspended until deferred calls triggered  by stack unwinding
         * have finished running.
         */
        SUSPENDED_BY_DEFERRED_DURING_STACK_UNWINDING,

        /*
         * Entered after execution hits the `halt` opcode.
         * Execution is stopped *immediately* after entering this state meaning
         * that the stack is not unwound, deferred calls are not invoked, etc.
         */
        HALTED,
    };

  private:
    std::vector<std::unique_ptr<Frame>> frames;
    STATE current_state = STATE::UNINITIALISED;

  public:
    std::string const entry_function;
    Process* parent_process;

    Op_address_type jump_base;
    Op_address_type instruction_pointer;

    std::unique_ptr<Frame> frame_new;
    using size_type = decltype(frames)::size_type;

    std::vector<std::unique_ptr<Try_frame>> tryframes;
    std::unique_ptr<Try_frame> try_frame_new;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set either by user code, or the VM.
     */
    std::unique_ptr<viua::types::Value> thrown;
    std::unique_ptr<viua::types::Value> caught;

    /*
     *  Global register set of parent process.
     */
    viua::kernel::Register_set* global_register_set;

    /*  Variables set after the VM has executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    std::unique_ptr<viua::types::Value> return_value;  // return value of
                                                       // top-most frame on the
                                                       // stack

    void adjust_instruction_pointer(const Try_frame*, std::string const);
    auto unwind_call_stack_to(const Frame*) -> void;
    auto unwind_try_stack_to(const Try_frame*) -> void;
    auto unwind_to(const Try_frame*, std::string const) -> void;
    auto find_catch_frame() -> std::tuple<Try_frame*, std::string>;

    auto set_return_value() -> void;

    auto state_of() const -> STATE;
    auto state_of(const STATE) -> STATE;

    viua::scheduler::Process_scheduler* attached_scheduler;

    auto bind(viua::kernel::Register_set*) -> void;

    /*
     * Iteration over and access to frames of the currently active stack in the
     * process.
     */
    auto begin() const -> decltype(frames.begin());
    auto end() const -> decltype(frames.end());
    auto at(decltype(frames)::size_type i) const -> decltype(frames.at(i));
    auto back() const -> decltype(frames.back());
    auto pop() -> std::unique_ptr<Frame>;
    auto emplace_back(std::unique_ptr<Frame> f)
        -> decltype(frames.emplace_back(f));

    /*
     * Access to information about the depth of the currently active stack in
     * the process.
     */
    auto size() const -> decltype(frames)::size_type;
    auto clear() -> void;  // FIXME is this used?

    auto register_deferred_calls_from(Frame*) -> void;
    auto register_deferred_calls(bool const = true) -> void;

    auto prepare_frame(viua::bytecode::codec::register_index_type const) -> Frame*;
    auto push_prepared_frame() -> void;

    auto adjust_jump_base_for_block(std::string const&)
        -> viua::internals::types::Op_address_type;
    auto adjust_jump_base_for(std::string const&)
        -> viua::internals::types::Op_address_type;
    auto unwind() -> void;

    Stack(std::string,
          Process*,
          viua::kernel::Register_set*,
          viua::scheduler::Process_scheduler*);

    static uint16_t const MAX_STACK_SIZE = 8192;
};

struct Decoder_adapter {
    viua::bytecode::codec::main::Decoder decoder;

    template<typename T>
    auto fetch_value_of(Op_address_type& addr, Process& proc) const -> T*
    {
        auto value = fetch_value(addr, proc);

        auto converted = dynamic_cast<T*>(value);
        if (converted == nullptr) {
            // FIXME don't use the old generic-exception type
            throw std::make_unique<viua::types::Exception>(
                ("fetched invalid type: expected '" + T::type_name
                 + "' but got '" + value->type() + "'")
                /* "Invalid_type" */
                /* , "expected " + T::type_name + ", got " + value->type() */
            );
        }

        return converted;
    }

    template<typename T>
    auto fetch_value_of_or_void(Op_address_type& addr, Process& proc) const
        -> std::optional<T*>
    {
        auto r = fetch_register_or_void(addr, proc);
        if (not r.has_value()) {
            return {};
        }

        auto value     = (*r)->get();
        auto converted = dynamic_cast<T*>(value);
        if (converted == nullptr) {
            // FIXME don't use the old generic-exception type
            throw std::make_unique<viua::types::Exception>(
                ("fetched invalid type: expected '" + T::type_name
                 + "' but got '" + value->type() + "'")
                /* "Invalid_type" */
                /* , "expected " + T::type_name + ", got " + value->type() */
            );
        }

        return converted;
    }

  private:
    auto fetch_slot(Op_address_type&) const
        -> viua::bytecode::codec::Register_access;

  public:
    auto fetch_register(Op_address_type&, Process&, bool const = false) const
        -> viua::kernel::Register*;
    auto fetch_register_or_void(Op_address_type&,
                                Process&,
                                bool const = false) const
        -> std::optional<viua::kernel::Register*>;
    auto fetch_tagged_register(Op_address_type&,
                               Process&,
                               bool const = false) const
        -> std::pair<viua::bytecode::codec::Register_set, viua::kernel::Register*>;
    auto fetch_register_index(Op_address_type&) const
        -> viua::bytecode::codec::register_index_type;
    auto fetch_tagged_register_index(Op_address_type&) const
        -> std::pair<viua::bytecode::codec::Register_set,
                     viua::bytecode::codec::register_index_type>;
    auto fetch_value(Op_address_type&, Process&) const -> viua::types::Value*;

    auto fetch_string(Op_address_type&) const -> std::string;
    auto fetch_bits_string(Op_address_type&) const -> std::vector<uint8_t>;
    auto fetch_timeout(Op_address_type&) const
        -> viua::bytecode::codec::timeout_type;
    auto fetch_float(Op_address_type&) const -> double;
    auto fetch_i32(Op_address_type&) const -> int32_t;
    auto fetch_bool(Op_address_type&) const -> bool;
    auto fetch_address(Op_address_type&) const -> uint64_t;
};

class Process {
#ifdef AS_DEBUG_HEADER
  public:
#endif
    friend Stack;
    /*
     * Variables set below control whether the VM should gather and
     * emit additional (debugging, profiling, tracing) information
     * regarding executed code.
     */
    bool const tracing_enabled;
    auto get_trace_line(uint8_t const*) const
        -> std::string;
    auto emit_trace_line(uint8_t const*) const -> void;

    /*
     * Pointer to scheduler the process is currently bound to.
     * This is not constant because processes may migrate between
     * schedulers during load balancing.
     */
    viua::scheduler::Process_scheduler* attached_scheduler;
    std::atomic<bool> is_pinned_to_scheduler = false;

  public:
    Decoder_adapter const decoder;

  private:
    /*
     * Parent process of this process.
     * May be null if this process has been detached.
     */
    viua::process::Process* parent_process;

    /*
     * Watchdog function is the final in-process defense mechanism against
     * failure. When an exception causes the whole active stack to be unwound it
     * means that the process was unable to handle the exception for whatever
     * reason and the handling is delegated to the watchdog function.
     *
     * The exception is wrapped in a struct, along with some other information
     * and passed as the first argument to the function set as watchdog.
     */
    std::string watchdog_function{""};
    std::unique_ptr<Frame> watchdog_frame;
    bool watchdog_failed{false};

    /*
     * Every process has its own global and static register sets. They are both
     * shared among each process' stack, though to facilitate quick exchange of
     * information between them if needed.
     */
    std::unique_ptr<viua::kernel::Register_set> global_register_set;
    std::map<std::string, std::unique_ptr<viua::kernel::Register_set>>
        static_registers;
    auto ensure_static_registers(std::string) -> void;


    /*
     * Call stack information.
     *
     * A process may have many suspended stacks (the simplest example: during
     * execution of a deferred function which itself has registered a deferred
     * call), and at most one active stack.
     *
     * A process always begins execution with exactly one stack. During the
     * execution of the main function of the process more stacks may be added.
     * The number of stacks in the process is dynamic and may change many times
     * during execution.
     *
     * A process has zero stacks when if finished execution but has not yet been
     * reaped.
     *
     * When one stack finishes running (pops off its last frame), the execution
     * returns to the "previous stack". In effect, there is a stack of stacks
     * inside every running process.
     */
    std::map<Stack*, std::unique_ptr<Stack>> stacks;
    Stack* stack;
    std::stack<Stack*> stacks_order;

    /*
     * Messages which the process already has locally. To avoid synchronisation
     * with the kernel on every receive operation, messages are buffered locally
     * and fetched from the kernel-held mailbox in batches.
     */
    std::queue<std::unique_ptr<viua::types::Value>> message_queue;

    /*  Methods dealing with stack and frame manipulation, and
     *  function calls.
     */
    auto request_new_frame(
        viua::bytecode::codec::register_index_type const arguments_size = 0)
        -> Frame*;
    auto request_new_try_frame() -> Try_frame*;
    auto push_frame() -> void;
    auto adjust_jump_base_for_block(std::string const&)
        -> viua::internals::types::Op_address_type;
    auto adjust_jump_base_for(std::string const&)
        -> viua::internals::types::Op_address_type;
    // call native (i.e. written in Viua) function
    auto call_native(Op_address_type,
                     std::string const&,
                     viua::kernel::Register* const,
                     std::string const&) -> Op_address_type;
    // call foreign (i.e. from a C++ extension) function
    auto call_foreign(Op_address_type,
                      std::string const&,
                      viua::kernel::Register* const,
                      std::string const&) -> Op_address_type;

    auto push_deferred(std::string const) -> void;

    std::atomic_bool finished;
    std::atomic_bool is_joinable;
    std::atomic_bool is_suspended;
    uint16_t process_priority;
    std::mutex process_mtx;

    /*  viua::process::Process identifier.
     */
    viua::process::PID process_id;
    bool is_hidden;

    /*
     * Timeouts for receiving messages, waiting for processes, and waiting for
     * I/O. They can be reused for all these things since for the duration of
     * each type of wait the process is suspended and is unable to execute any
     * code.
     */
    std::chrono::steady_clock::time_point waiting_until;
    bool timeout_active      = false;
    bool wait_until_infinity = false;

    /*  Methods implementing individual instructions.
     */
    auto opizero(Op_address_type) -> Op_address_type;
    auto opinteger(Op_address_type) -> Op_address_type;
    auto opiinc(Op_address_type) -> Op_address_type;
    auto opidec(Op_address_type) -> Op_address_type;

    auto opfloat(Op_address_type) -> Op_address_type;

    auto opitof(Op_address_type) -> Op_address_type;
    auto opftoi(Op_address_type) -> Op_address_type;
    auto opstoi(Op_address_type) -> Op_address_type;
    auto opstof(Op_address_type) -> Op_address_type;

    auto opadd(Op_address_type) -> Op_address_type;
    auto opsub(Op_address_type) -> Op_address_type;
    auto opmul(Op_address_type) -> Op_address_type;
    auto opdiv(Op_address_type) -> Op_address_type;
    auto oplt(Op_address_type) -> Op_address_type;
    auto oplte(Op_address_type) -> Op_address_type;
    auto opgt(Op_address_type) -> Op_address_type;
    auto opgte(Op_address_type) -> Op_address_type;
    auto opeq(Op_address_type) -> Op_address_type;

    auto opstring(Op_address_type) -> Op_address_type;
    auto opstreq(Op_address_type) -> Op_address_type;

    auto optext(Op_address_type) -> Op_address_type;
    auto optexteq(Op_address_type) -> Op_address_type;
    auto optextat(Op_address_type) -> Op_address_type;
    auto optextsub(Op_address_type) -> Op_address_type;
    auto optextlength(Op_address_type) -> Op_address_type;
    auto optextcommonprefix(Op_address_type) -> Op_address_type;
    auto optextcommonsuffix(Op_address_type) -> Op_address_type;
    auto optextconcat(Op_address_type) -> Op_address_type;

    auto opvector(Op_address_type) -> Op_address_type;
    auto opvinsert(Op_address_type) -> Op_address_type;
    auto opvpush(Op_address_type) -> Op_address_type;
    auto opvpop(Op_address_type) -> Op_address_type;
    auto opvat(Op_address_type) -> Op_address_type;
    auto opvlen(Op_address_type) -> Op_address_type;

    auto opboolean(Op_address_type) -> Op_address_type;
    auto opnot(Op_address_type) -> Op_address_type;
    auto opand(Op_address_type) -> Op_address_type;
    auto opor(Op_address_type) -> Op_address_type;

    auto opbits_of_integer(Op_address_type) -> Op_address_type;
    auto opinteger_of_bits(Op_address_type) -> Op_address_type;

    auto opbits(Op_address_type) -> Op_address_type;
    auto opbitand(Op_address_type) -> Op_address_type;
    auto opbitor(Op_address_type) -> Op_address_type;
    auto opbitnot(Op_address_type) -> Op_address_type;
    auto opbitxor(Op_address_type) -> Op_address_type;
    auto opbitat(Op_address_type) -> Op_address_type;
    auto opbitset(Op_address_type) -> Op_address_type;
    auto opshl(Op_address_type) -> Op_address_type;
    auto opshr(Op_address_type) -> Op_address_type;
    auto opashl(Op_address_type) -> Op_address_type;
    auto opashr(Op_address_type) -> Op_address_type;
    auto oprol(Op_address_type) -> Op_address_type;
    auto opror(Op_address_type) -> Op_address_type;

    auto opwrapincrement(Op_address_type) -> Op_address_type;
    auto opwrapdecrement(Op_address_type) -> Op_address_type;
    auto opwrapadd(Op_address_type) -> Op_address_type;
    auto opwrapsub(Op_address_type) -> Op_address_type;
    auto opwrapmul(Op_address_type) -> Op_address_type;
    auto opwrapdiv(Op_address_type) -> Op_address_type;

    auto opcheckedsincrement(Op_address_type) -> Op_address_type;
    auto opcheckedsdecrement(Op_address_type) -> Op_address_type;
    auto opcheckedsadd(Op_address_type) -> Op_address_type;
    auto opcheckedssub(Op_address_type) -> Op_address_type;
    auto opcheckedsmul(Op_address_type) -> Op_address_type;
    auto opcheckedsdiv(Op_address_type) -> Op_address_type;

    auto opcheckeduincrement(Op_address_type) -> Op_address_type;
    auto opcheckedudecrement(Op_address_type) -> Op_address_type;
    auto opcheckeduadd(Op_address_type) -> Op_address_type;
    auto opcheckedusub(Op_address_type) -> Op_address_type;
    auto opcheckedumul(Op_address_type) -> Op_address_type;
    auto opcheckedudiv(Op_address_type) -> Op_address_type;

    auto opsaturatingsincrement(Op_address_type) -> Op_address_type;
    auto opsaturatingsdecrement(Op_address_type) -> Op_address_type;
    auto opsaturatingsadd(Op_address_type) -> Op_address_type;
    auto opsaturatingssub(Op_address_type) -> Op_address_type;
    auto opsaturatingsmul(Op_address_type) -> Op_address_type;
    auto opsaturatingsdiv(Op_address_type) -> Op_address_type;

    auto opsaturatinguincrement(Op_address_type) -> Op_address_type;
    auto opsaturatingudecrement(Op_address_type) -> Op_address_type;
    auto opsaturatinguadd(Op_address_type) -> Op_address_type;
    auto opsaturatingusub(Op_address_type) -> Op_address_type;
    auto opsaturatingumul(Op_address_type) -> Op_address_type;
    auto opsaturatingudiv(Op_address_type) -> Op_address_type;

    auto opmove(Op_address_type) -> Op_address_type;
    auto opcopy(Op_address_type) -> Op_address_type;
    auto opptr(Op_address_type) -> Op_address_type;
    auto opptrlive(Op_address_type) -> Op_address_type;
    auto opswap(Op_address_type) -> Op_address_type;
    auto opdelete(Op_address_type) -> Op_address_type;
    auto opisnull(Op_address_type) -> Op_address_type;

    auto opprint(Op_address_type) -> Op_address_type;
    auto opecho(Op_address_type) -> Op_address_type;

    auto opcapture(Op_address_type) -> Op_address_type;
    auto opcapturecopy(Op_address_type) -> Op_address_type;
    auto opcapturemove(Op_address_type) -> Op_address_type;
    auto opclosure(Op_address_type) -> Op_address_type;

    auto opfunction(Op_address_type) -> Op_address_type;

    auto opframe(Op_address_type) -> Op_address_type;
    auto opparam(Op_address_type) -> Op_address_type;
    auto oppamv(Op_address_type) -> Op_address_type;
    auto oparg(Op_address_type) -> Op_address_type;
    auto opallocate_registers(Op_address_type) -> Op_address_type;

    auto opcall(Op_address_type) -> Op_address_type;
    auto optailcall(Op_address_type) -> Op_address_type;
    auto opdefer(Op_address_type) -> Op_address_type;
    auto opprocess(Op_address_type) -> Op_address_type;
    auto opself(Op_address_type) -> Op_address_type;
    auto oppideq(Op_address_type) -> Op_address_type;
    auto opjoin(Op_address_type) -> Op_address_type;
    auto opsend(Op_address_type) -> Op_address_type;
    auto opreceive(Op_address_type) -> Op_address_type;
    auto opwatchdog(Op_address_type) -> Op_address_type;
    auto opreturn(Op_address_type) -> Op_address_type;

    auto opjump(Op_address_type) -> Op_address_type;
    auto opif(Op_address_type) -> Op_address_type;

    auto optry(Op_address_type) -> Op_address_type;
    auto opcatch(Op_address_type) -> Op_address_type;
    auto opdraw(Op_address_type) -> Op_address_type;
    auto openter(Op_address_type) -> Op_address_type;
    auto opthrow(Op_address_type) -> Op_address_type;
    auto opleave(Op_address_type) -> Op_address_type;

    auto opatom(Op_address_type) -> Op_address_type;
    auto opatomeq(Op_address_type) -> Op_address_type;

    auto opstruct(Op_address_type) -> Op_address_type;
    auto opstructinsert(Op_address_type) -> Op_address_type;
    auto opstructremove(Op_address_type) -> Op_address_type;
    auto opstructat(Op_address_type) -> Op_address_type;
    auto opstructkeys(Op_address_type) -> Op_address_type;

    auto opimport(Op_address_type) -> Op_address_type;

    auto op_io_read(Op_address_type) -> Op_address_type;
    auto op_io_write(Op_address_type) -> Op_address_type;
    auto op_io_close(Op_address_type) -> Op_address_type;
    auto op_io_wait(Op_address_type) -> Op_address_type;
    auto op_io_cancel(Op_address_type) -> Op_address_type;

  public:
    auto dispatch(Op_address_type) -> Op_address_type;
    auto tick() -> Op_address_type;

    auto register_at(viua::bytecode::codec::register_index_type,
                     viua::bytecode::codec::Register_set) -> viua::kernel::Register*;

    bool joinable() const;
    void join();
    void detach();

    void suspend();
    void wakeup();
    bool suspended() const;

    auto parent() const -> viua::process::Process*;
    auto starting_function() const -> std::string;

    auto pass(std::unique_ptr<viua::types::Value>) -> void;

    auto priority() const -> decltype(process_priority);
    auto priority(decltype(process_priority) p) -> void;

    auto stopped() const -> bool;

    auto terminated() const -> bool;
    auto get_active_exception() -> viua::types::Value*;
    auto transfer_active_exception() -> std::unique_ptr<viua::types::Value>;
    auto raise(std::unique_ptr<viua::types::Value>) -> void;
    auto handle_active_exception() -> void;

    auto migrate_to(viua::scheduler::Process_scheduler*) -> void;

    auto get_return_value() -> std::unique_ptr<viua::types::Value>;

    auto watchdogged() const -> bool;
    auto watchdog() const -> std::string;
    auto frame_for_watchdog() -> std::unique_ptr<Frame>;

    auto become(std::string const&, std::unique_ptr<Frame>) -> Op_address_type;

    auto start() -> Op_address_type;
    auto execution_at() const -> decltype(Stack::instruction_pointer);

    auto trace() const -> std::vector<Frame*>;

    auto pid() const -> viua::process::PID;
    auto hidden() const -> bool;
    auto hidden(bool) -> void;

    auto empty() const -> bool;

    auto pin(bool const x = true) -> void { is_pinned_to_scheduler = x; }
    auto pinned() const -> bool { return is_pinned_to_scheduler; }

    auto schedule_io(std::unique_ptr<viua::scheduler::io::IO_interaction>)
        -> void;
    auto cancel_io(std::tuple<uint64_t, uint64_t> const) -> void;

    Process(std::unique_ptr<Frame>,
            viua::scheduler::Process_scheduler*,
            viua::process::Process*,
            bool const             = false,
            Decoder_adapter const& = Decoder_adapter{});
    ~Process();

    static viua::bytecode::codec::register_index_type const DEFAULT_REGISTER_SIZE =
        255;
};
}}  // namespace viua::process


#endif
