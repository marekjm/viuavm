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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/function.h>
#include <viua/types/closure.h>
#include <viua/types/reference.h>
#include <viua/exceptions.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* viua::process::Process::opcapture(byte* addr) {
    /** Capture object by reference.
     */
    unsigned target_closure_register = 0, target_register = 0, source_register = 0;
    tie(addr, target_closure_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, target_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    auto target_closure = static_cast<viua::types::Closure*>(fetch(target_closure_register));
    if (target_register >= target_closure->regset->size()) {
        throw new viua::types::Exception("cannot capture object: register index out exceeded size of closure register set");
    }

    auto captured_object = uregset->at(source_register);
    auto rf = dynamic_cast<viua::types::Reference*>(captured_object);
    if (rf == nullptr) {
        // turn captured object into a reference to take it out of VM's default
        // memory management scheme and put it under reference-counting scheme
        // this is needed to bind the captured object's life to lifetime of the closure
        rf = new viua::types::Reference(captured_object);
        uregset->empty(source_register);    // empty - do not delete the captured object or SEGFAULTS will follow
        uregset->set(source_register, rf);  // set the register to contain the newly-created reference
    }
    target_closure->regset->set(target_register, rf->copy());

    return addr;
}

byte* viua::process::Process::opcapturecopy(byte* addr) {
    /** Capture object by copy.
     */
    unsigned target_closure_register = 0, target_register = 0;
    tie(addr, target_closure_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, target_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Type* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    auto target_closure = static_cast<viua::types::Closure*>(fetch(target_closure_register));
    if (target_register >= target_closure->regset->size()) {
        throw new viua::types::Exception("cannot capture object: register index out exceeded size of closure register set");
    }

    target_closure->regset->set(target_register, source->copy());

    return addr;
}

byte* viua::process::Process::opcapturemove(byte* addr) {
    /** Capture object by move.
     */
    unsigned target_closure_register = 0, target_register = 0, source_register = 0;
    tie(addr, target_closure_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, target_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    auto target_closure = static_cast<viua::types::Closure*>(fetch(target_closure_register));
    if (target_register >= target_closure->regset->size()) {
        throw new viua::types::Exception("cannot capture object: register index out exceeded size of closure register set");
    }

    target_closure->regset->set(target_register, uregset->pop(source_register));

    return addr;
}

byte* viua::process::Process::opclosure(byte* addr) {
    /** Create a closure from a function.
     */
    if (uregset != frames.back()->regset.get()) {
        throw new viua::types::Exception("creating closures from nonlocal registers is forbidden");
    }

    unsigned target = 0;
    string function_name;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, function_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    place(target, new viua::types::Closure(function_name, new viua::kernel::RegisterSet(uregset->size())));

    return addr;
}

byte* viua::process::Process::opfunction(byte* addr) {
    /** Create function object in a register.
     *
     *  Such objects can be used to call functions, and
     *  are can be used to pass functions as parameters and
     *  return them from other functions.
     */
    unsigned target = 0;
    string function_name;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, function_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    place(target, new viua::types::Function(function_name));

    return addr;
}

byte* viua::process::Process::opfcall(byte* addr) {
    /*  Call a function object.
     */
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    unsigned return_register = 0;

    if (not return_void) {
        tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Type* fn_source = nullptr;
    tie(addr, fn_source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    // FIXME: there should be a check it this is *really* a function object
    auto fn = static_cast<viua::types::Function*>(fn_source);

    string call_name = fn->name();

    if (not scheduler->isNativeFunction(call_name)) {
        throw new viua::types::Exception("fcall to undefined function: " + call_name);
    }

    byte* call_address = nullptr;
    call_address = adjustJumpBaseFor(call_name);

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new viua::types::Exception("fcall without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->return_void = return_void;
    frame_new->place_return_value_in = return_register;

    if (fn->type() == "Closure") {
        frame_new->setLocalRegisterSet(static_cast<viua::types::Closure*>(fn)->regset.get(), false);
    }

    pushFrame();

    return call_address;
}
