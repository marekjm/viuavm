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


const viua::internals::types::register_index DEFAULT_REGISTER_SIZE = 255;
const uint16_t MAX_STACK_SIZE = 8192;


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

            // Static registers
            std::map<std::string, std::unique_ptr<viua::kernel::RegisterSet>> static_registers;


            // Call stack
            viua::internals::types::byte* jump_base;
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

            viua::internals::types::bytecode_size instruction_counter;
            viua::internals::types::byte* instruction_pointer;

            std::queue<std::unique_ptr<viua::types::Type>> message_queue;

            viua::types::Type* fetch(viua::internals::types::register_index) const;
            std::unique_ptr<viua::types::Type> pop(viua::internals::types::register_index);
            void place(viua::internals::types::register_index, std::unique_ptr<viua::types::Type>);
            void ensureStaticRegisters(std::string);

            /*  Methods dealing with stack and frame manipulation, and
             *  function calls.
             */
            Frame* requestNewFrame(viua::internals::types::register_index arguments_size = 0, viua::internals::types::register_index registers_size = 0);
            TryFrame* requestNewTryFrame();
            void pushFrame();
            void dropFrame();
            viua::internals::types::byte* adjustJumpBaseForBlock(const std::string&);
            viua::internals::types::byte* adjustJumpBaseFor(const std::string&);
            // call native (i.e. written in Viua) function
            viua::internals::types::byte* callNative(viua::internals::types::byte*, const std::string&, viua::kernel::Register*, const std::string&);
            // call foreign (i.e. from a C++ extension) function
            viua::internals::types::byte* callForeign(viua::internals::types::byte*, const std::string&, viua::kernel::Register*, const std::string&);
            // call foreign method (i.e. method of a pure-C++ class loaded into machine's typesystem)
            viua::internals::types::byte* callForeignMethod(viua::internals::types::byte*, viua::types::Type*, const std::string&, viua::kernel::Register*, const std::string&);

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
            bool timeout_active = false;
            bool wait_until_infinity = false;

            /*  Methods implementing individual instructions.
             */
            viua::internals::types::byte* opizero(viua::internals::types::byte*);
            viua::internals::types::byte* opistore(viua::internals::types::byte*);
            viua::internals::types::byte* opiinc(viua::internals::types::byte*);
            viua::internals::types::byte* opidec(viua::internals::types::byte*);

            viua::internals::types::byte* opfstore(viua::internals::types::byte*);

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

            viua::internals::types::byte* opstrstore(viua::internals::types::byte*);

            viua::internals::types::byte* opvec(viua::internals::types::byte*);
            viua::internals::types::byte* opvinsert(viua::internals::types::byte*);
            viua::internals::types::byte* opvpush(viua::internals::types::byte*);
            viua::internals::types::byte* opvpop(viua::internals::types::byte*);
            viua::internals::types::byte* opvat(viua::internals::types::byte*);
            viua::internals::types::byte* opvlen(viua::internals::types::byte*);

            viua::internals::types::byte* boolean(viua::internals::types::byte*);
            viua::internals::types::byte* opnot(viua::internals::types::byte*);
            viua::internals::types::byte* opand(viua::internals::types::byte*);
            viua::internals::types::byte* opor(viua::internals::types::byte*);

            viua::internals::types::byte* opmove(viua::internals::types::byte*);
            viua::internals::types::byte* opcopy(viua::internals::types::byte*);
            viua::internals::types::byte* opptr(viua::internals::types::byte*);
            viua::internals::types::byte* opswap(viua::internals::types::byte*);
            viua::internals::types::byte* opdelete(viua::internals::types::byte*);
            viua::internals::types::byte* opisnull(viua::internals::types::byte*);

            viua::internals::types::byte* opress(viua::internals::types::byte*);

            viua::internals::types::byte* opprint(viua::internals::types::byte*);
            viua::internals::types::byte* opecho(viua::internals::types::byte*);

            viua::internals::types::byte* opcapture(viua::internals::types::byte*);
            viua::internals::types::byte* opcapturecopy(viua::internals::types::byte*);
            viua::internals::types::byte* opcapturemove(viua::internals::types::byte*);
            viua::internals::types::byte* opclosure(viua::internals::types::byte*);

            viua::internals::types::byte* opfunction(viua::internals::types::byte*);
            viua::internals::types::byte* opfcall(viua::internals::types::byte*);

            viua::internals::types::byte* opframe(viua::internals::types::byte*);
            viua::internals::types::byte* opparam(viua::internals::types::byte*);
            viua::internals::types::byte* oppamv(viua::internals::types::byte*);
            viua::internals::types::byte* oparg(viua::internals::types::byte*);
            viua::internals::types::byte* opargc(viua::internals::types::byte*);

            viua::internals::types::byte* opcall(viua::internals::types::byte*);
            viua::internals::types::byte* optailcall(viua::internals::types::byte*);
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

            viua::internals::types::byte* opclass(viua::internals::types::byte*);
            viua::internals::types::byte* opprototype(viua::internals::types::byte*);
            viua::internals::types::byte* opderive(viua::internals::types::byte*);
            viua::internals::types::byte* opattach(viua::internals::types::byte*);
            viua::internals::types::byte* opregister(viua::internals::types::byte*);

            viua::internals::types::byte* opnew(viua::internals::types::byte*);
            viua::internals::types::byte* opmsg(viua::internals::types::byte*);
            viua::internals::types::byte* opinsert(viua::internals::types::byte*);
            viua::internals::types::byte* opremove(viua::internals::types::byte*);

            viua::internals::types::byte* opimport(viua::internals::types::byte*);
            viua::internals::types::byte* oplink(viua::internals::types::byte*);

            public:
                viua::internals::types::byte* dispatch(viua::internals::types::byte*);
                viua::internals::types::byte* tick();

                viua::types::Type* obtain(viua::internals::types::register_index) const;
                void put(viua::internals::types::register_index, std::unique_ptr<viua::types::Type>);

                viua::kernel::Register* register_at(viua::internals::types::register_index);
                viua::kernel::Register* register_at(viua::internals::types::register_index, viua::internals::RegisterSets);

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
                viua::internals::types::byte* become(const std::string&, std::unique_ptr<Frame>);

                viua::internals::types::byte* begin();
                auto counter() const -> decltype(instruction_counter);
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
