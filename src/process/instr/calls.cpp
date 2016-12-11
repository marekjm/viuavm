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

#include <memory>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* viua::process::Process::opframe(byte* addr) {
    /** Create new frame for function calls.
     */
    unsigned arguments = 0, local_registers = 0;
    tie(addr, arguments) = viua::bytecode::decoder::operands::fetch_primitive_uint(addr, this);
    tie(addr, local_registers) = viua::bytecode::decoder::operands::fetch_primitive_uint(addr, this);

    requestNewFrame(arguments, local_registers);

    return addr;
}

byte* viua::process::Process::opparam(byte* addr) {
    /** Run param instruction.
     */
    viua::internals::types::register_index parameter_no_operand_index = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Type *source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    if (parameter_no_operand_index >= frame_new->arguments->size()) {
        throw new viua::types::Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    frame_new->arguments->set(parameter_no_operand_index, std::move(source->copy()));
    frame_new->arguments->clear(parameter_no_operand_index);

    return addr;
}

byte* viua::process::Process::oppamv(byte* addr) {
    /** Run pamv instruction.
     */
    viua::internals::types::register_index parameter_no_operand_index = 0, source = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= frame_new->arguments->size()) {
        throw new viua::types::Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    frame_new->arguments->set(parameter_no_operand_index, std::move(currently_used_register_set->pop(source)));
    frame_new->arguments->clear(parameter_no_operand_index);
    frame_new->arguments->flag(parameter_no_operand_index, MOVED);

    return addr;
}

byte* viua::process::Process::oparg(byte* addr) {
    /** Run arg instruction.
     */
    viua::internals::types::register_index destination_register_index = 0, parameter_no_operand_index = 0;
    bool destination_is_void = viua::bytecode::decoder::operands::is_void(addr);

    if (not destination_is_void) {
        tie(addr, destination_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= frames.back()->arguments->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: " << parameter_no_operand_index;
        throw new viua::types::Exception(oss.str());
    }

    unique_ptr<viua::types::Type> argument;

    if (frames.back()->arguments->isflagged(parameter_no_operand_index, MOVED)) {
        argument = std::move(frames.back()->arguments->pop(parameter_no_operand_index));
    } else {
        argument = std::move(frames.back()->arguments->get(parameter_no_operand_index)->copy());
    }

    if (not destination_is_void) {
        currently_used_register_set->set(destination_register_index, std::move(argument));
    }

    return addr;
}

byte* viua::process::Process::opargc(byte* addr) {
    viua::internals::types::register_index target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    currently_used_register_set->set(target, unique_ptr<viua::types::Type>{new viua::types::Integer(static_cast<int>(frames.back()->arguments->size()))});

    return addr;
}

byte* viua::process::Process::opcall(byte* addr) {
    /*  Run call instruction.
     */
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    viua::internals::types::register_index return_register = 0;

    if (not return_void) {
        tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

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
        if (frame_new->arguments->size() == 0) {
            throw new viua::types::Exception("cannot call foreign method using empty frame");
        }
        if (frame_new->arguments->at(0) == nullptr) {
            throw new viua::types::Exception("frame must have at least one argument when used to call a foreign method");
        }
        auto obj = frame_new->arguments->at(0);
        return callForeignMethod(addr, obj, call_name, return_void, return_register, call_name);
    }

    auto caller = (is_native ? &viua::process::Process::callNative : &viua::process::Process::callForeign);
    return (this->*caller)(addr, call_name, return_void, return_register, "");
}

byte* viua::process::Process::optailcall(byte* addr) {
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

    last_frame->arguments = std::move(frame_new->arguments);

    // new frame must be deleted to prevent future errors
    // it's a simulated "push-and-pop" from the stack
    frame_new.reset(nullptr);

    return adjustJumpBaseFor(call_name);
}

byte* viua::process::Process::opreturn(byte* addr) {
    if (frames.size() == 0) {
        throw new viua::types::Exception("no frame on stack: no call to return from");
    }
    addr = frames.back()->ret_address();

    unique_ptr<viua::types::Type> returned;
    viua::internals::types::register_index return_value_register = frames.back()->place_return_value_in;
    if (return_value_register != 0 and not frames.back()->return_void) {
        // we check in 0. register because it's reserved for return values
        if (currently_used_register_set->at(0) == nullptr) {
            throw new viua::types::Exception("return value requested by frame but function did not set return register");
        }
        returned = std::move(currently_used_register_set->pop(0));
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        place(return_value_register, std::move(returned));
    }

    if (frames.size() > 0) {
        adjustJumpBaseFor(frames.back()->function_name);
    }

    return addr;
}
