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
#include <viua/types/function.h>
#include <viua/types/closure.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opframe(viua::internals::types::byte* addr) {
    /** Create new frame for function calls.
     */
    viua::internals::types::register_index arguments = 0, local_registers = 0;
    tie(addr, arguments) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, local_registers) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    requestNewFrame(arguments, local_registers);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opparam(viua::internals::types::byte* addr) {
    /** Run param instruction.
     */
    viua::internals::types::register_index parameter_no_operand_index = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Type *source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    if (parameter_no_operand_index >= stack.frame_new->arguments->size()) {
        throw new viua::types::Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    stack.frame_new->arguments->set(parameter_no_operand_index, source->copy());
    stack.frame_new->arguments->clear(parameter_no_operand_index);

    return addr;
}

viua::internals::types::byte* viua::process::Process::oppamv(viua::internals::types::byte* addr) {
    /** Run pamv instruction.
     */
    viua::internals::types::register_index parameter_no_operand_index = 0, source = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= stack.frame_new->arguments->size()) {
        throw new viua::types::Exception("parameter register index out of bounds (greater than arguments set size) while adding parameter");
    }
    stack.frame_new->arguments->set(parameter_no_operand_index, currently_used_register_set->pop(source));
    stack.frame_new->arguments->clear(parameter_no_operand_index);
    stack.frame_new->arguments->flag(parameter_no_operand_index, MOVED);

    return addr;
}

viua::internals::types::byte* viua::process::Process::oparg(viua::internals::types::byte* addr) {
    /** Run arg instruction.
     */
    viua::kernel::Register *target = nullptr;
    bool destination_is_void = viua::bytecode::decoder::operands::is_void(addr);

    if (not destination_is_void) {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::internals::types::register_index parameter_no_operand_index = 0;
    tie(addr, parameter_no_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= stack.back()->arguments->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: " << parameter_no_operand_index;
        throw new viua::types::Exception(oss.str());
    }

    unique_ptr<viua::types::Type> argument;

    if (stack.back()->arguments->isflagged(parameter_no_operand_index, MOVED)) {
        argument = stack.back()->arguments->pop(parameter_no_operand_index);
    } else {
        argument = stack.back()->arguments->get(parameter_no_operand_index)->copy();
    }

    if (not destination_is_void) {
        *target = std::move(argument);
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opargc(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Integer(static_cast<int>(stack.back()->arguments->size()))};

    return addr;
}

viua::internals::types::byte* viua::process::Process::opcall(viua::internals::types::byte* addr) {
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* return_register = nullptr;

    if (not return_void) {
        tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    string call_name;
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Type* fn_source = nullptr;
        tie(addr, fn_source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

        auto fn = dynamic_cast<viua::types::Function*>(fn_source);
        if (not fn) {
            throw new viua::types::Exception("type is not callable: " + fn_source->type());
        }
        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack.frame_new->setLocalRegisterSet(static_cast<viua::types::Closure*>(fn)->rs(), false);
        }
    } else {
        tie(addr, call_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

    bool is_native = scheduler->isNativeFunction(call_name);
    bool is_foreign = scheduler->isForeignFunction(call_name);
    bool is_foreign_method = scheduler->isForeignMethod(call_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new viua::types::Exception("call to undefined function: " + call_name);
    }

    if (is_foreign_method) {
        if (stack.frame_new == nullptr) {
            throw new viua::types::Exception("cannot call foreign method without a frame");
        }
        if (stack.frame_new->arguments->size() == 0) {
            throw new viua::types::Exception("cannot call foreign method using empty frame");
        }
        if (stack.frame_new->arguments->at(0) == nullptr) {
            throw new viua::types::Exception("frame must have at least one argument when used to call a foreign method");
        }
        auto obj = stack.frame_new->arguments->at(0);
        return callForeignMethod(addr, obj, call_name, return_register, call_name);
    }

    auto caller = (is_native ? &viua::process::Process::callNative : &viua::process::Process::callForeign);
    return (this->*caller)(addr, call_name, return_register, "");
}

viua::internals::types::byte* viua::process::Process::optailcall(viua::internals::types::byte* addr) {
    /*  Run tailcall instruction.
     */
    string call_name;
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Type* fn_source = nullptr;
        tie(addr, fn_source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

        auto fn = dynamic_cast<viua::types::Function*>(fn_source);
        if (not fn) {
            throw new viua::types::Exception("type is not callable: " + fn_source->type());
        }
        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack.back()->local_register_set.reset(static_cast<viua::types::Closure*>(fn)->give());
            currently_used_register_set = stack.back()->local_register_set.get();
        }
    } else {
        tie(addr, call_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

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

    stack.back()->arguments = std::move(stack.frame_new->arguments);

    // new frame must be deleted to prevent future errors
    // it's a simulated "push-and-pop" from the stack
    stack.frame_new.reset(nullptr);

    return adjustJumpBaseFor(call_name);
}

viua::internals::types::byte* viua::process::Process::opreturn(viua::internals::types::byte* addr) {
    if (stack.size() == 0) {
        throw new viua::types::Exception("no frame on stack: no call to return from");
    }
    addr = stack.back()->ret_address();

    unique_ptr<viua::types::Type> returned;
    viua::kernel::Register* return_register = stack.back()->return_register;
    if (return_register != nullptr) {
        // we check in 0. register because it's reserved for return values
        if (currently_used_register_set->at(0) == nullptr) {
            throw new viua::types::Exception("return value requested by frame but function did not set return register");
        }
        returned = currently_used_register_set->pop(0);
    }

    stack.pop();

    // place return value
    if (returned and stack.size() > 0) {
        *return_register = std::move(returned);
    }

    if (stack.size() > 0) {
        adjustJumpBaseFor(stack.back()->function_name);
    }

    return addr;
}
