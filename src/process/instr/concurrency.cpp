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

#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::opprocess(byte* addr) {
    /*  Run process instruction.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string call_name = viua::operand::extractString(addr);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    frame_new->function_name = call_name;

    bool disown = (target == 0);
    auto spawned_process = scheduler->spawn(std::move(frame_new), this, disown);
    if (not disown) {
        place(target, new ProcessType(spawned_process));
    }

    return addr;
}
byte* Process::opjoin(byte* addr) {
    /** Join a process.
     *
     *  This opcode blocks execution of current process until
     *  the process being joined finishes execution.
     */
    byte* return_addr = (addr-1);

    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    if (ProcessType* thrd = dynamic_cast<ProcessType*>(fetch(source))) {
        if (thrd->stopped()) {
            thrd->join();
            return_addr = addr;
            if (thrd->terminated()) {
                thrown = thrd->transferActiveException();
            }
            if (target) {
                place(target, thrd->getReturnValue().release());
            }
        }
    } else {
        throw new Exception("invalid type: expected Process");
    }

    return return_addr;
}
byte* Process::opsend(byte* addr) {
    /** Send a message to a process.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    if (ProcessType* thrd = dynamic_cast<ProcessType*>(fetch(target))) {
        scheduler->send(thrd->pid(), unique_ptr<Type>(pop(source)));
    } else {
        throw new Exception("invalid type: expected Process");
    }

    return addr;
}
byte* Process::opreceive(byte* addr) {
    /** Receive a message.
     *
     *  This opcode blocks execution of current process
     *  until a message arrives.
     */
    byte* return_addr = (addr-1);

    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (not is_hidden) {
        scheduler->receive(process_id, message_queue);
    }

    if (not message_queue.empty()) {
        place(target, message_queue.front().release());
        message_queue.pop();
        return_addr = addr;
    } else {
        if (is_hidden) {
            suspend();
        }
    }

    return return_addr;
}
byte* Process::opwatchdog(byte* addr) {
    /*  Run watchdog instruction.
     */
    string call_name = viua::operand::extractString(addr);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("watchdog process from undefined function: " + call_name);
    }
    if (not is_native) {
        throw new Exception("watchdog process must be a native function, used foreign " + call_name);
    }

    frame_new->function_name = call_name;
    scheduler->spawnWatchdog(std::move(frame_new));

    return addr;
}
