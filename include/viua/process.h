/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/tryframe.h>
#include <viua/pid.h>
#include <viua/types/value.h>


class Halt_exception : public std::runtime_error {
  public:
    Halt_exception() : std::runtime_error("execution halted") {}
};


namespace viua { namespace scheduler {
class Virtual_process_scheduler;
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

    viua::scheduler::Virtual_process_scheduler* scheduler;

    auto bind(viua::kernel::Register_set*) -> void;

    auto begin() const -> decltype(frames.begin());
    auto end() const -> decltype(frames.end());

    auto at(decltype(frames)::size_type i) const -> decltype(frames.at(i));
    auto back() const -> decltype(frames.back());

    auto register_deferred_calls_from(Frame*) -> void;
    auto register_deferred_calls(bool const = true) -> void;
    auto pop() -> std::unique_ptr<Frame>;

    auto size() const -> decltype(frames)::size_type;
    auto clear() -> void;

    auto emplace_back(std::unique_ptr<Frame> f)
        -> decltype(frames.emplace_back(f));

    auto prepare_frame(viua::internals::types::register_index,
                       viua::internals::types::register_index) -> Frame*;
    auto push_prepared_frame() -> void;

    auto adjust_jump_base_for_block(std::string const&)
        -> viua::internals::types::Op_address_type;
    auto adjust_jump_base_for(std::string const&)
        -> viua::internals::types::Op_address_type;
    auto unwind() -> void;

    Stack(std::string,
          Process*,
          viua::kernel::Register_set*,
          viua::scheduler::Virtual_process_scheduler*);

    static uint16_t const MAX_STACK_SIZE = 8192;
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
    auto get_trace_line(viua::internals::types::byte const*) const
        -> std::string;
    auto emit_trace_line(viua::internals::types::byte const*) const -> void;

    /*
     * Pointer to scheduler the process is currently bound to.
     * This is not constant because processes may migrate between
     * schedulers during load balancing.
     */
    viua::scheduler::Virtual_process_scheduler* scheduler;

    /*
     * Parent process of this process.
     * May be null if this process has been detached.
     */
    viua::process::Process* parent_process;

    std::string watchdog_function{""};
    bool watchdog_failed{false};

    std::unique_ptr<viua::kernel::Register_set> global_register_set;

    // Static registers
    std::map<std::string, std::unique_ptr<viua::kernel::Register_set>>
        static_registers;


    // Call stack
    std::map<Stack*, std::unique_ptr<Stack>> stacks;
    Stack* stack;
    std::stack<Stack*> stacks_order;

    std::queue<std::unique_ptr<viua::types::Value>> message_queue;

    void ensure_static_registers(std::string);

    /*  Methods dealing with stack and frame manipulation, and
     *  function calls.
     */
    Frame* request_new_frame(
        viua::internals::types::register_index arguments_size = 0,
        viua::internals::types::register_index registers_size = 0);
    Try_frame* request_new_try_frame();
    void push_frame();
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
    viua::internals::types::process_time_slice_type process_priority;
    std::mutex process_mtx;

    /*  viua::process::Process identifier.
     */
    viua::process::PID process_id;
    bool is_hidden;

    /*  Timeouts for message passing, and
     *  multiprocessing.
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
    auto opargc(Op_address_type) -> Op_address_type;
    auto opallocate_registers(Op_address_type) -> Op_address_type;

    auto opcall(Op_address_type) -> Op_address_type;
    auto optailcall(Op_address_type) -> Op_address_type;
    auto opdefer(Op_address_type) -> Op_address_type;
    auto opprocess(Op_address_type) -> Op_address_type;
    auto opself(Op_address_type) -> Op_address_type;
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
    auto opstructkeys(Op_address_type) -> Op_address_type;

    auto opimport(Op_address_type) -> Op_address_type;

  public:
    auto dispatch(Op_address_type) -> Op_address_type;
    auto tick() -> Op_address_type;

    viua::kernel::Register* register_at(viua::internals::types::register_index,
                                        viua::internals::Register_sets);

    bool joinable() const;
    void join();
    void detach();

    void suspend();
    void wakeup();
    bool suspended() const;

    viua::process::Process* parent() const;
    std::string starting_function() const;

    void pass(std::unique_ptr<viua::types::Value>);

    auto priority() const -> decltype(process_priority);
    void priority(decltype(process_priority) p);

    bool stopped() const;

    bool terminated() const;
    viua::types::Value* get_active_exception();
    std::unique_ptr<viua::types::Value> transfer_active_exception();
    void raise(std::unique_ptr<viua::types::Value>);
    void handle_active_exception();

    void migrate_to(viua::scheduler::Virtual_process_scheduler*);

    std::unique_ptr<viua::types::Value> get_return_value();

    bool watchdogged() const;
    std::string watchdog() const;
    auto become(std::string const&, std::unique_ptr<Frame>) -> Op_address_type;

    auto begin() -> Op_address_type;
    auto execution_at() const -> decltype(Stack::instruction_pointer);

    std::vector<Frame*> trace() const;

    viua::process::PID pid() const;
    bool hidden() const;
    void hidden(bool);

    bool empty() const;

    Process(std::unique_ptr<Frame>,
            viua::scheduler::Virtual_process_scheduler*,
            viua::process::Process*,
            bool const = false);
    ~Process();

    static viua::internals::types::register_index const DEFAULT_REGISTER_SIZE =
        255;
};
}}  // namespace viua::process


#endif
