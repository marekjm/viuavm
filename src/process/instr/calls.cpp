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

#include <viua/bytecode/decoder/operands.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::opframe(byte* addr) {
    /** Create new frame for function calls.
     */
    unsigned arguments = 0, local_registers = 0;
    tie(addr, arguments) = viua::bytecode::decoder::operands::fetch_primitive_uint(addr, this);
    tie(addr, local_registers) = viua::bytecode::decoder::operands::fetch_primitive_uint(addr, this);

    requestNewFrame(arguments, local_registers);

    return addr;
}

byte* Process::opparam(byte* addr) {
    /** Run param instruction.
     */
    unsigned parameter_no_operand_index = 0, source = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= frame_new->args->size()) {
        throw new viua::types::Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    frame_new->args->set(parameter_no_operand_index, fetch(source)->copy());
    frame_new->args->clear(parameter_no_operand_index);

    return addr;
}

byte* Process::oppamv(byte* addr) {
    /** Run pamv instruction.
     */
    unsigned parameter_no_operand_index = 0, source = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= frame_new->args->size()) {
        throw new viua::types::Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    frame_new->args->set(parameter_no_operand_index, uregset->pop(source));
    frame_new->args->clear(parameter_no_operand_index);
    frame_new->args->flag(parameter_no_operand_index, MOVED);

    return addr;
}

byte* Process::oparg(byte* addr) {
    /** Run arg instruction.
     */
    unsigned destination_register_index = 0, parameter_no_operand_index = 0;
    tie(addr, destination_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= frames.back()->args->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: " << parameter_no_operand_index;
        throw new viua::types::Exception(oss.str());
    }

    if (frames.back()->args->isflagged(parameter_no_operand_index, MOVED)) {
        uregset->set(destination_register_index, frames.back()->args->pop(parameter_no_operand_index));
    } else {
        uregset->set(destination_register_index, frames.back()->args->get(parameter_no_operand_index)->copy());
    }

    return addr;
}

byte* Process::opargc(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    uregset->set(target, new viua::types::Integer(static_cast<int>(frames.back()->args->size())));

    return addr;
}

byte* Process::opcall(byte* addr) {
    /*  Run call instruction.
     */
    unsigned return_register = 0;
    tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    string call_name;
    tie(addr, call_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);
    bool is_foreign_method = scheduler->isForeignMethod(call_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new viua::types::Exception("call to undefined function: " + call_name);
    }

    if (is_foreign_method) {
        if (frame_new == nullptr) {
            throw new viua::types::Exception("cannot call foreign method without a frame");
        }
        if (frame_new->args->size() == 0) {
            throw new viua::types::Exception("cannot call foreign method using empty frame");
        }
        if (frame_new->args->at(0) == nullptr) {
            throw new viua::types::Exception("frame must have at least one argument when used to call a foreign method");
        }
        auto obj = frame_new->args->at(0);
        return callForeignMethod(addr, obj, call_name, return_register, call_name);
    }

    auto caller = (is_native ? &Process::callNative : &Process::callForeign);
    return (this->*caller)(addr, call_name, return_register, "");
}

byte* Process::optailcall(byte* addr) {
    /*  Run tailcall instruction.
     */
    string call_name;
    tie(addr, call_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);
    bool is_foreign_method = scheduler->isForeignMethod(call_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new viua::types::Exception("tail call to undefined function: " + call_name);
    }
    // FIXME: make to possible to tail call foreign functions and methods
    if (not is_native) {
        throw new viua::types::Exception("tail call to non-native function: " + call_name);
    }

    Frame *last_frame = frames.back().get();

    // move arguments from new frame to old frame
    delete last_frame->args;
    last_frame->args = frame_new->args;
    frame_new->args = nullptr;

    // new frame must be deleted to prevent future errors
    // it's a simulated "push-and-pop" from the stack
    frame_new.reset(nullptr);

    return adjustJumpBaseFor(call_name);
}

byte* Process::opreturn(byte* addr) {
    if (frames.size() == 0) {
        throw new viua::types::Exception("no frame on stack: no call to return from");
    }
    addr = frames.back()->ret_address();

    viua::types::Type* returned = nullptr;
    unsigned return_value_register = frames.back()->place_return_value_in;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new viua::types::Exception("return value requested by frame but function did not set return register");
        }
        returned = uregset->pop(0);
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        place(return_value_register, returned);
    }

    if (frames.size() > 0) {
        adjustJumpBaseFor(frames.back()->function_name);
    }

    return addr;
}
