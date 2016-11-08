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

#include <chrono>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* viua::process::Process::opprocess(byte* addr) {
    /*  Run process instruction.
     */
    unsigned target = 0;
    bool target_is_void = viua::bytecode::decoder::operands::is_void(addr);

    if (not target_is_void) {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    string call_name;
    tie(addr, call_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);

    if (not (is_native or is_foreign)) {
        throw new viua::types::Exception("call to undefined function: " + call_name);
    }

    frame_new->function_name = call_name;

    auto spawned_process = scheduler->spawn(std::move(frame_new), this, target_is_void);
    if (not target_is_void) {
        place(target, new viua::types::Process(spawned_process));
    }

    return addr;
}
byte* viua::process::Process::opjoin(byte* addr) {
    /** Join a process.
     *
     *  This opcode blocks execution of current process until
     *  the process being joined finishes execution.
     */
    byte* return_addr = (addr-1);

    unsigned target = 0;
    bool target_is_void = viua::bytecode::decoder::operands::is_void(addr);

    if (not target_is_void) {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    unsigned source = 0, timeout = 0;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, timeout) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (timeout and not timeout_active) {
        waiting_until = (std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout-1));
        timeout_active = true;
    } else if (not timeout and not timeout_active) {
        wait_until_infinity = true;
        timeout_active = true;
    }

    if (auto thrd = dynamic_cast<viua::types::Process*>(fetch(source))) {
        if (thrd->stopped()) {
            thrd->join();
            return_addr = addr;
            if (thrd->terminated()) {
                thrown = thrd->transferActiveException();
            }
            if (not target_is_void) {
                place(target, thrd->getReturnValue().release());
            }
        } else if (timeout_active and (not wait_until_infinity) and (waiting_until < std::chrono::steady_clock::now())) {
            timeout_active = false;
            wait_until_infinity = false;
            thrown.reset(new viua::types::Exception("process did not join"));
            return_addr = addr;
        }
    } else {
        throw new viua::types::Exception("invalid type: expected viua::process::Process");
    }

    return return_addr;
}
byte* viua::process::Process::opsend(byte* addr) {
    /** Send a message to a process.
     */
    unsigned target = 0, source = 0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (auto thrd = dynamic_cast<viua::types::Process*>(fetch(target))) {
        scheduler->send(thrd->pid(), unique_ptr<viua::types::Type>(pop(source)));
    } else {
        throw new viua::types::Exception("invalid type: expected viua::process::Process");
    }

    return addr;
}
byte* viua::process::Process::opreceive(byte* addr) {
    /** Receive a message.
     *
     *  This opcode blocks execution of current process
     *  until a message arrives.
     */
    byte* return_addr = (addr-1);

    unsigned target = 0, timeout = 0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, timeout) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (timeout and not timeout_active) {
        waiting_until = (std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout-1));
        timeout_active = true;
    } else if (not timeout and not timeout_active) {
        wait_until_infinity = true;
        timeout_active = true;
    }

    if (not is_hidden) {
        scheduler->receive(process_id, message_queue);
    }

    if (not message_queue.empty()) {
        if (target) {
            place(target, message_queue.front().release());
        }
        message_queue.pop();
        timeout_active = false;
        wait_until_infinity = false;
        return_addr = addr;
    } else {
        if (is_hidden) {
            suspend();
        }
        if (timeout_active and (not wait_until_infinity) and (waiting_until < std::chrono::steady_clock::now())) {
            timeout_active = false;
            wait_until_infinity = false;
            thrown.reset(new viua::types::Exception("no message received"));
            return_addr = addr;
        }
    }

    return return_addr;
}
byte* viua::process::Process::opwatchdog(byte* addr) {
    /*  Run watchdog instruction.
     */
    string call_name;
    tie(addr, call_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);

    if (not (is_native or is_foreign)) {
        throw new viua::types::Exception("watchdog process from undefined function: " + call_name);
    }
    if (not is_native) {
        throw new viua::types::Exception("watchdog process must be a native function, used foreign " + call_name);
    }

    if (not watchdog_function.empty()) {
        throw new viua::types::Exception("watchdog already set");
    }

    watchdog_function = call_name;

    return addr;
}
byte* viua::process::Process::opself(byte* addr) {
    /*  Run process instruction.
     */
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(target, new viua::types::Process(this));

    return addr;
}
