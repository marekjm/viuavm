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

#ifndef VIUA_PROCESS_H
#define VIUA_PROCESS_H

#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/prototype.h>
#include <viua/cpu/registerset.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/tryframe.h>
#include <viua/include/module.h>


const unsigned DEFAULT_REGISTER_SIZE = 256;
const unsigned MAX_STACK_SIZE = 8192;


class HaltException : public std::runtime_error {
    public:
        HaltException(): std::runtime_error("execution halted") {}
};


namespace viua {
    namespace scheduler {
        class VirtualProcessScheduler;
    }
}


class Process;


class PID {
    const Process *associated_process;

    public:
    bool operator==(const PID&) const;
    bool operator==(const Process*) const;
    bool operator<(const PID&) const;
    bool operator>(const PID&) const;

    auto get() const -> decltype(associated_process);

    PID(const Process*);
};

class Process {
#ifdef AS_DEBUG_HEADER
    public:
#endif
    viua::scheduler::VirtualProcessScheduler *scheduler;

    Process* parent_process;
    const std::string entry_function;

    // Global register set
    std::unique_ptr<RegisterSet> regset;

    // Currently used register set
    RegisterSet* uregset;

    // Temporary register
    std::unique_ptr<Type> tmp;

    // Static registers
    std::map<std::string, std::unique_ptr<RegisterSet>> static_registers;


    // Call stack
    byte* jump_base;
    std::vector<std::unique_ptr<Frame>> frames;
    std::unique_ptr<Frame> frame_new;

    // Block stack
    std::vector<std::unique_ptr<TryFrame>> tryframes;
    std::unique_ptr<TryFrame> try_frame_new;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set either by user code, or the VM.
     */
    std::unique_ptr<Type> thrown;
    std::unique_ptr<Type> caught;

    /*  Variables set after the VM has executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    std::unique_ptr<Type> return_value; // return value of top-most frame on the stack

    // FIXME: change unsigned to uint64_t
    uint64_t instruction_counter;
    byte* instruction_pointer;

    std::queue<std::unique_ptr<Type>> message_queue;

    Type* fetch(unsigned) const;
    Type* pop(unsigned);
    void place(unsigned, Type*);
    void updaterefs(Type*, Type*);
    bool hasrefs(unsigned);
    void ensureStaticRegisters(std::string);

    /*  Methods dealing with stack and frame manipulation, and
     *  function calls.
     */
    Frame* requestNewFrame(unsigned arguments_size = 0, unsigned registers_size = 0);
    TryFrame* requestNewTryFrame();
    void pushFrame();
    void dropFrame();
    byte* adjustJumpBaseForBlock(const std::string&);
    byte* adjustJumpBaseFor(const std::string&);
    // call native (i.e. written in Viua) function
    byte* callNative(byte*, const std::string&, const bool, const unsigned, const std::string&);
    // call foreign (i.e. from a C++ extension) function
    byte* callForeign(byte*, const std::string&, const bool, const unsigned, const std::string&);
    // call foreign method (i.e. method of a pure-C++ class loaded into machine's typesystem)
    byte* callForeignMethod(byte*, Type*, const std::string&, const bool, const unsigned, const std::string&);

    /*  Stack unwinding methods.
     */
    void adjustInstructionPointer(TryFrame*, std::string);
    void unwindCallStack(TryFrame*);
    void unwindTryStack(TryFrame*);
    void unwindStack(std::tuple<TryFrame*, std::string>);
    void unwindStack(TryFrame*, std::string);
    std::tuple<TryFrame*, std::string> findCatchFrame();

    bool finished;
    std::atomic_bool is_joinable;
    std::atomic_bool is_suspended;
    unsigned process_priority;
    std::mutex process_mtx;

    /*  Process identifier.
     */
    PID process_id;
    bool is_hidden;

    /*  Methods implementing individual instructions.
     */
    byte* opizero(byte*);
    byte* opistore(byte*);
    byte* opiadd(byte*);
    byte* opisub(byte*);
    byte* opimul(byte*);
    byte* opidiv(byte*);

    byte* opilt(byte*);
    byte* opilte(byte*);
    byte* opigt(byte*);
    byte* opigte(byte*);
    byte* opieq(byte*);

    byte* opiinc(byte*);
    byte* opidec(byte*);

    byte* opfstore(byte*);
    byte* opfadd(byte*);
    byte* opfsub(byte*);
    byte* opfmul(byte*);
    byte* opfdiv(byte*);

    byte* opflt(byte*);
    byte* opflte(byte*);
    byte* opfgt(byte*);
    byte* opfgte(byte*);
    byte* opfeq(byte*);

    byte* opbstore(byte*);

    byte* opitof(byte*);
    byte* opftoi(byte*);
    byte* opstoi(byte*);
    byte* opstof(byte*);

    byte* opstrstore(byte*);

    byte* opvec(byte*);
    byte* opvinsert(byte*);
    byte* opvpush(byte*);
    byte* opvpop(byte*);
    byte* opvat(byte*);
    byte* opvlen(byte*);

    byte* boolean(byte*);
    byte* opnot(byte*);
    byte* opand(byte*);
    byte* opor(byte*);

    byte* opmove(byte*);
    byte* opcopy(byte*);
    byte* opptr(byte*);
    byte* opswap(byte*);
    byte* opdelete(byte*);
    byte* opisnull(byte*);

    byte* opress(byte*);
    byte* optmpri(byte*);
    byte* optmpro(byte*);

    byte* opprint(byte*);
    byte* opecho(byte*);

    byte* openclose(byte*);
    byte* openclosecopy(byte*);
    byte* openclosemove(byte*);
    byte* opclosure(byte*);

    byte* opfunction(byte*);
    byte* opfcall(byte*);

    byte* opframe(byte*);
    byte* opparam(byte*);
    byte* oppamv(byte*);
    byte* oparg(byte*);
    byte* opargc(byte*);

    byte* opcall(byte*);
    byte* optailcall(byte*);
    byte* opprocess(byte*);
    byte* opjoin(byte*);
    byte* opreceive(byte*);
    byte* opwatchdog(byte*);
    byte* opreturn(byte*);

    byte* opjump(byte*);
    byte* opbranch(byte*);

    byte* optry(byte*);
    byte* opcatch(byte*);
    byte* oppull(byte*);
    byte* openter(byte*);
    byte* opthrow(byte*);
    byte* opleave(byte*);

    byte* opclass(byte*);
    byte* opprototype(byte*);
    byte* opderive(byte*);
    byte* opattach(byte*);
    byte* opregister(byte*);

    byte* opnew(byte*);
    byte* opmsg(byte*);
    byte* opinsert(byte*);
    byte* opremove(byte*);

    byte* opimport(byte*);
    byte* oplink(byte*);

    public:
        byte* dispatch(byte*);
        byte* tick();

        Type* obtain(unsigned) const;
        void put(unsigned, Type*);

        bool joinable() const;
        void join();
        void detach();

        void suspend();
        void wakeup();
        bool suspended() const;

        Process* parent() const;
        std::string starting_function() const;

        void pass(std::unique_ptr<Type>);

        auto priority() const -> decltype(process_priority);
        void priority(decltype(process_priority) p);

        bool stopped() const;

        bool terminated() const;
        Type* getActiveException();
        std::unique_ptr<Type> transferActiveException();
        void raiseException(Type*);
        void handleActiveException();

        void popFrame();
        std::unique_ptr<Type> getReturnValue();

        byte* begin();
        uint64_t counter() const;
        auto executionAt() const -> decltype(instruction_pointer);

        std::vector<Frame*> trace() const;

        PID pid() const;
        bool hidden() const;
        void hidden(bool);

        bool empty() const;

        Process(std::unique_ptr<Frame>, viua::scheduler::VirtualProcessScheduler*, Process*);
        ~Process();
};


#endif
