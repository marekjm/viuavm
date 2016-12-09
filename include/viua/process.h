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
#include <chrono>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/prototype.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/tryframe.h>
#include <viua/include/module.h>
#include <viua/pid.h>


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

namespace viua {
    namespace process {
        class Process {
#ifdef AS_DEBUG_HEADER
            public:
#endif
            viua::scheduler::VirtualProcessScheduler *scheduler;

            viua::process::Process* parent_process;
            const std::string entry_function;

            std::string watchdog_function { "" };
            bool watchdog_failed { false };

            std::unique_ptr<viua::kernel::RegisterSet> global_register_set;

            /*
             * This pointer points different register sets during the process's lifetime.
             * It can be explicitly adjusted by the user code (using "ress" instruction), or
             * implicitly by the VM (e.g. when calling a closure).
             */
            viua::kernel::RegisterSet* currently_used_register_set;

            // Temporary register
            std::unique_ptr<viua::types::Type> tmp;

            // Static registers
            std::map<std::string, std::unique_ptr<viua::kernel::RegisterSet>> static_registers;


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
            std::unique_ptr<viua::types::Type> thrown;
            std::unique_ptr<viua::types::Type> caught;

            /*  Variables set after the VM has executed bytecode.
             *  They describe exit conditions of the bytecode that just stopped running.
             */
            std::unique_ptr<viua::types::Type> return_value; // return value of top-most frame on the stack

            uint64_t instruction_counter;
            byte* instruction_pointer;

            std::queue<std::unique_ptr<viua::types::Type>> message_queue;

            viua::types::Type* fetch(unsigned) const;
            std::unique_ptr<viua::types::Type> pop(unsigned);
            void place(unsigned, std::unique_ptr<viua::types::Type>);
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
            byte* callForeignMethod(byte*, viua::types::Type*, const std::string&, const bool, const unsigned, const std::string&);

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

            /*  viua::process::Process identifier.
             */
            viua::process::PID process_id;
            bool is_hidden;

            /*  Timeouts for message passing, and
             *  multiprocessing.
             */
            std::chrono::steady_clock::time_point waiting_until;
            bool timeout_active = false;
            bool wait_until_infinity = false;

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

            byte* opitof(byte*);
            byte* opftoi(byte*);
            byte* opstoi(byte*);
            byte* opstof(byte*);

            byte* opadd(byte*);

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

            byte* opcapture(byte*);
            byte* opcapturecopy(byte*);
            byte* opcapturemove(byte*);
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
            byte* opself(byte*);
            byte* opjoin(byte*);
            byte* opsend(byte*);
            byte* opreceive(byte*);
            byte* opwatchdog(byte*);
            byte* opreturn(byte*);

            byte* opjump(byte*);
            byte* opif(byte*);

            byte* optry(byte*);
            byte* opcatch(byte*);
            byte* opdraw(byte*);
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

                viua::types::Type* obtain(unsigned) const;
                void put(unsigned, std::unique_ptr<viua::types::Type>);

                bool joinable() const;
                void join();
                void detach();

                void suspend();
                void wakeup();
                bool suspended() const;

                viua::process::Process* parent() const;
                std::string starting_function() const;

                void pass(std::unique_ptr<viua::types::Type>);

                auto priority() const -> decltype(process_priority);
                void priority(decltype(process_priority) p);

                bool stopped() const;

                bool terminated() const;
                viua::types::Type* getActiveException();
                std::unique_ptr<viua::types::Type> transferActiveException();
                void raise(std::unique_ptr<viua::types::Type>);
                void handleActiveException();

                void migrate_to(viua::scheduler::VirtualProcessScheduler*);

                void popFrame();
                std::unique_ptr<viua::types::Type> getReturnValue();

                bool watchdogged() const;
                std::string watchdog() const;
                byte* become(const std::string&, std::unique_ptr<Frame>);

                byte* begin();
                uint64_t counter() const;
                auto executionAt() const -> decltype(instruction_pointer);

                std::vector<Frame*> trace() const;

                viua::process::PID pid() const;
                bool hidden() const;
                void hidden(bool);

                bool empty() const;

                Process(std::unique_ptr<Frame>, viua::scheduler::VirtualProcessScheduler*, viua::process::Process*);
                ~Process();
        };
    }
}


#endif
