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


class HaltException : public std::runtime_error {
  public:
    HaltException() : std::runtime_error("execution halted") {}
};


namespace viua { namespace scheduler {
class VirtualProcessScheduler;
}}  // namespace viua::scheduler

namespace viua { namespace process {
class Process;

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
    const std::string entry_function;
    Process* parent_process;

    viua::internals::types::byte* jump_base;
    viua::internals::types::byte* instruction_pointer;

    std::unique_ptr<Frame> frame_new;
    using size_type = decltype(frames)::size_type;

    std::vector<std::unique_ptr<TryFrame>> tryframes;
    std::unique_ptr<TryFrame> try_frame_new;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set either by user code, or the VM.
     */
    std::unique_ptr<viua::types::Value> thrown;
    std::unique_ptr<viua::types::Value> caught;

    /*
     *  Currently used register, and
     *  global register set of parent process.
     */
    viua::kernel::RegisterSet** currently_used_register_set;
    viua::kernel::RegisterSet* global_register_set;

    /*  Variables set after the VM has executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    std::unique_ptr<viua::types::Value> return_value;  // return value of
                                                       // top-most frame on the
                                                       // stack

    void adjust_instruction_pointer(const TryFrame*, const std::string);
    auto unwind_call_stack_to(const Frame*) -> void;
    auto unwind_try_stack_to(const TryFrame*) -> void;
    auto unwind_to(const TryFrame*, const std::string) -> void;
    auto find_catch_frame() -> std::tuple<TryFrame*, std::string>;

  public:
    auto set_return_value() -> void;

    auto state_of() const -> STATE;
    auto state_of(const STATE) -> STATE;

    viua::scheduler::VirtualProcessScheduler* scheduler;

    auto bind(viua::kernel::RegisterSet**, viua::kernel::RegisterSet*) -> void;

    auto begin() const -> decltype(frames.begin());
    auto end() const -> decltype(frames.end());

    auto at(decltype(frames)::size_type i) const -> decltype(frames.at(i));
    auto back() const -> decltype(frames.back());

    auto register_deferred_calls_from(Frame*) -> void;
    auto register_deferred_calls(const bool = true) -> void;
    auto pop() -> std::unique_ptr<Frame>;

    auto size() const -> decltype(frames)::size_type;
    auto clear() -> void;

    auto emplace_back(std::unique_ptr<Frame> f)
        -> decltype(frames.emplace_back(f));

    auto prepare_frame(viua::internals::types::register_index,
                       viua::internals::types::register_index) -> Frame*;
    auto push_prepared_frame() -> void;

    viua::internals::types::byte* adjust_jump_base_for_block(
        const std::string&);
    viua::internals::types::byte* adjust_jump_base_for(const std::string&);
    auto unwind() -> void;

    Stack(std::string,
          Process*,
          viua::kernel::RegisterSet**,
          viua::kernel::RegisterSet*,
          viua::scheduler::VirtualProcessScheduler*);

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
    const bool tracing_enabled;
    auto get_trace_line(viua::internals::types::byte*) const -> std::string;
    auto emit_trace_line(viua::internals::types::byte*) const -> void;

    /*
     * Pointer to scheduler the process is currently bound to.
     * This is not constant because processes may migrate between
     * schedulers during load balancing.
     */
    viua::scheduler::VirtualProcessScheduler* scheduler;

    /*
     * Parent process of this process.
     * May be null if this process has been detached.
     */
    viua::process::Process* parent_process;

    std::string watchdog_function{""};
    bool watchdog_failed{false};

    std::unique_ptr<viua::kernel::RegisterSet> global_register_set;

    /*
     * This pointer points different register sets during the process's
     * lifetime. It can be explicitly adjusted by the user code (using "ress"
     * instruction), or implicitly by the VM (e.g. when calling a closure).
     * FIXME Remove this. It is not needed after "ress" was removed. The
     * "current" pseudo-register set must also be removed for this to be viable.
     */
    viua::kernel::RegisterSet* currently_used_register_set;

    // Static registers
    std::map<std::string, std::unique_ptr<viua::kernel::RegisterSet>>
        static_registers;


    // Call stack
    std::map<Stack*, std::unique_ptr<Stack>> stacks;
    Stack* stack;
    std::stack<Stack*> stacks_order;

    std::queue<std::unique_ptr<viua::types::Value>> message_queue;

    viua::types::Value* fetch(viua::internals::types::register_index) const;
    std::unique_ptr<viua::types::Value> pop(
        viua::internals::types::register_index);
    void place(viua::internals::types::register_index,
               std::unique_ptr<viua::types::Value>);
    void ensure_static_registers(std::string);

    /*  Methods dealing with stack and frame manipulation, and
     *  function calls.
     */
    Frame* request_new_frame(
        viua::internals::types::register_index arguments_size = 0,
        viua::internals::types::register_index registers_size = 0);
    TryFrame* request_new_try_frame();
    void push_frame();
    viua::internals::types::byte* adjust_jump_base_for_block(
        const std::string&);
    viua::internals::types::byte* adjust_jump_base_for(const std::string&);
    // call native (i.e. written in Viua) function
    viua::internals::types::byte* call_native(viua::internals::types::byte*,
                                              const std::string&,
                                              viua::kernel::Register*,
                                              const std::string&);
    // call foreign (i.e. from a C++ extension) function
    viua::internals::types::byte* call_foreign(viua::internals::types::byte*,
                                               const std::string&,
                                               viua::kernel::Register*,
                                               const std::string&);
    // call foreign method (i.e. method of a pure-C++ class loaded into
    // machine's typesystem)
    viua::internals::types::byte* call_foreign_method(
        viua::internals::types::byte*,
        viua::types::Value*,
        const std::string&,
        viua::kernel::Register*,
        const std::string&);

    auto push_deferred(std::string) -> void;

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
    viua::internals::types::byte* opizero(viua::internals::types::byte*);
    viua::internals::types::byte* opinteger(viua::internals::types::byte*);
    viua::internals::types::byte* opiinc(viua::internals::types::byte*);
    viua::internals::types::byte* opidec(viua::internals::types::byte*);

    viua::internals::types::byte* opfloat(viua::internals::types::byte*);

    viua::internals::types::byte* opitof(viua::internals::types::byte*);
    viua::internals::types::byte* opftoi(viua::internals::types::byte*);
    viua::internals::types::byte* opstoi(viua::internals::types::byte*);
    viua::internals::types::byte* opstof(viua::internals::types::byte*);

    viua::internals::types::byte* opadd(viua::internals::types::byte*);
    viua::internals::types::byte* opsub(viua::internals::types::byte*);
    viua::internals::types::byte* opmul(viua::internals::types::byte*);
    viua::internals::types::byte* opdiv(viua::internals::types::byte*);
    viua::internals::types::byte* oplt(viua::internals::types::byte*);
    viua::internals::types::byte* oplte(viua::internals::types::byte*);
    viua::internals::types::byte* opgt(viua::internals::types::byte*);
    viua::internals::types::byte* opgte(viua::internals::types::byte*);
    viua::internals::types::byte* opeq(viua::internals::types::byte*);

    viua::internals::types::byte* opstring(viua::internals::types::byte*);

    viua::internals::types::byte* optext(viua::internals::types::byte*);
    viua::internals::types::byte* optexteq(viua::internals::types::byte*);
    viua::internals::types::byte* optextat(viua::internals::types::byte*);
    viua::internals::types::byte* optextsub(viua::internals::types::byte*);
    viua::internals::types::byte* optextlength(viua::internals::types::byte*);
    viua::internals::types::byte* optextcommonprefix(
        viua::internals::types::byte*);
    viua::internals::types::byte* optextcommonsuffix(
        viua::internals::types::byte*);
    viua::internals::types::byte* optextconcat(viua::internals::types::byte*);

    viua::internals::types::byte* opvector(viua::internals::types::byte*);
    viua::internals::types::byte* opvinsert(viua::internals::types::byte*);
    viua::internals::types::byte* opvpush(viua::internals::types::byte*);
    viua::internals::types::byte* opvpop(viua::internals::types::byte*);
    viua::internals::types::byte* opvat(viua::internals::types::byte*);
    viua::internals::types::byte* opvlen(viua::internals::types::byte*);

    viua::internals::types::byte* boolean(viua::internals::types::byte*);
    viua::internals::types::byte* opnot(viua::internals::types::byte*);
    viua::internals::types::byte* opand(viua::internals::types::byte*);
    viua::internals::types::byte* opor(viua::internals::types::byte*);

    viua::internals::types::byte* opbits(viua::internals::types::byte*);
    viua::internals::types::byte* opbitand(viua::internals::types::byte*);
    viua::internals::types::byte* opbitor(viua::internals::types::byte*);
    viua::internals::types::byte* opbitnot(viua::internals::types::byte*);
    viua::internals::types::byte* opbitxor(viua::internals::types::byte*);
    viua::internals::types::byte* opbitat(viua::internals::types::byte*);
    viua::internals::types::byte* opbitset(viua::internals::types::byte*);
    viua::internals::types::byte* opshl(viua::internals::types::byte*);
    viua::internals::types::byte* opshr(viua::internals::types::byte*);
    viua::internals::types::byte* opashl(viua::internals::types::byte*);
    viua::internals::types::byte* opashr(viua::internals::types::byte*);
    viua::internals::types::byte* oprol(viua::internals::types::byte*);
    viua::internals::types::byte* opror(viua::internals::types::byte*);

    viua::internals::types::byte* opwrapincrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opwrapdecrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opwrapadd(viua::internals::types::byte*);
    viua::internals::types::byte* opwrapsub(viua::internals::types::byte*);
    viua::internals::types::byte* opwrapmul(viua::internals::types::byte*);
    viua::internals::types::byte* opwrapdiv(viua::internals::types::byte*);

    viua::internals::types::byte* opcheckedsincrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedsdecrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedsadd(viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedssub(viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedsmul(viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedsdiv(viua::internals::types::byte*);

    viua::internals::types::byte* opcheckeduincrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedudecrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opcheckeduadd(viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedusub(viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedumul(viua::internals::types::byte*);
    viua::internals::types::byte* opcheckedudiv(viua::internals::types::byte*);

    viua::internals::types::byte* opsaturatingsincrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingsdecrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingsadd(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingssub(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingsmul(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingsdiv(
        viua::internals::types::byte*);

    viua::internals::types::byte* opsaturatinguincrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingudecrement(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatinguadd(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingusub(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingumul(
        viua::internals::types::byte*);
    viua::internals::types::byte* opsaturatingudiv(
        viua::internals::types::byte*);

    viua::internals::types::byte* opmove(viua::internals::types::byte*);
    viua::internals::types::byte* opcopy(viua::internals::types::byte*);
    viua::internals::types::byte* opptr(viua::internals::types::byte*);
    viua::internals::types::byte* opptrlive(viua::internals::types::byte*);
    viua::internals::types::byte* opswap(viua::internals::types::byte*);
    viua::internals::types::byte* opdelete(viua::internals::types::byte*);
    viua::internals::types::byte* opisnull(viua::internals::types::byte*);

    viua::internals::types::byte* opprint(viua::internals::types::byte*);
    viua::internals::types::byte* opecho(viua::internals::types::byte*);

    viua::internals::types::byte* opcapture(viua::internals::types::byte*);
    viua::internals::types::byte* opcapturecopy(viua::internals::types::byte*);
    viua::internals::types::byte* opcapturemove(viua::internals::types::byte*);
    viua::internals::types::byte* opclosure(viua::internals::types::byte*);

    viua::internals::types::byte* opfunction(viua::internals::types::byte*);

    viua::internals::types::byte* opframe(viua::internals::types::byte*);
    viua::internals::types::byte* opparam(viua::internals::types::byte*);
    viua::internals::types::byte* oppamv(viua::internals::types::byte*);
    viua::internals::types::byte* oparg(viua::internals::types::byte*);
    viua::internals::types::byte* opargc(viua::internals::types::byte*);

    viua::internals::types::byte* opcall(viua::internals::types::byte*);
    viua::internals::types::byte* optailcall(viua::internals::types::byte*);
    viua::internals::types::byte* opdefer(viua::internals::types::byte*);
    viua::internals::types::byte* opprocess(viua::internals::types::byte*);
    viua::internals::types::byte* opself(viua::internals::types::byte*);
    viua::internals::types::byte* opjoin(viua::internals::types::byte*);
    viua::internals::types::byte* opsend(viua::internals::types::byte*);
    viua::internals::types::byte* opreceive(viua::internals::types::byte*);
    viua::internals::types::byte* opwatchdog(viua::internals::types::byte*);
    viua::internals::types::byte* opreturn(viua::internals::types::byte*);

    viua::internals::types::byte* opjump(viua::internals::types::byte*);
    viua::internals::types::byte* opif(viua::internals::types::byte*);

    viua::internals::types::byte* optry(viua::internals::types::byte*);
    viua::internals::types::byte* opcatch(viua::internals::types::byte*);
    viua::internals::types::byte* opdraw(viua::internals::types::byte*);
    viua::internals::types::byte* openter(viua::internals::types::byte*);
    viua::internals::types::byte* opthrow(viua::internals::types::byte*);
    viua::internals::types::byte* opleave(viua::internals::types::byte*);

    viua::internals::types::byte* opatom(viua::internals::types::byte*);
    viua::internals::types::byte* opatomeq(viua::internals::types::byte*);

    viua::internals::types::byte* opstruct(viua::internals::types::byte*);
    viua::internals::types::byte* opstructinsert(viua::internals::types::byte*);
    viua::internals::types::byte* opstructremove(viua::internals::types::byte*);
    viua::internals::types::byte* opstructkeys(viua::internals::types::byte*);

    viua::internals::types::byte* opimport(viua::internals::types::byte*);

  public:
    viua::internals::types::byte* dispatch(viua::internals::types::byte*);
    viua::internals::types::byte* tick();

    viua::types::Value* obtain(viua::internals::types::register_index) const;
    void put(viua::internals::types::register_index,
             std::unique_ptr<viua::types::Value>);

    viua::kernel::Register* register_at(viua::internals::types::register_index);
    viua::kernel::Register* register_at(viua::internals::types::register_index,
                                        viua::internals::RegisterSets);

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

    void migrate_to(viua::scheduler::VirtualProcessScheduler*);

    std::unique_ptr<viua::types::Value> get_return_value();

    bool watchdogged() const;
    std::string watchdog() const;
    viua::internals::types::byte* become(const std::string&,
                                         std::unique_ptr<Frame>);

    viua::internals::types::byte* begin();
    auto execution_at() const -> decltype(Stack::instruction_pointer);

    std::vector<Frame*> trace() const;

    viua::process::PID pid() const;
    bool hidden() const;
    void hidden(bool);

    bool empty() const;

    Process(std::unique_ptr<Frame>,
            viua::scheduler::VirtualProcessScheduler*,
            viua::process::Process*,
            const bool = false);
    ~Process();

    static viua::internals::types::register_index const DEFAULT_REGISTER_SIZE =
        255;
};
}}  // namespace viua::process


#endif
