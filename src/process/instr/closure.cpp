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


viua::internals::types::byte* viua::process::Process::opcapture(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::internals::types::register_index target_register = 0;
    tie(addr, target_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::kernel::Register* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    auto target_closure = static_cast<viua::types::Closure*>(target->get());
    if (target_register >= target_closure->rs()->size()) {
        throw new viua::types::Exception("cannot capture object: register index out exceeded size of closure register set");
    }

    auto captured_object = source->get();
    auto rf = dynamic_cast<viua::types::Reference*>(captured_object);
    if (rf == nullptr) {
        // turn captured object into a reference to take it out of VM's default
        // memory management scheme and put it under reference-counting scheme
        // this is needed to bind the captured object's life to lifetime of the closure
        rf = new viua::types::Reference(nullptr);
        rf->rebind(source->give());
        *source = unique_ptr<viua::types::Type>{rf};  // set the register to contain the newly-created reference
    }
    target_closure->rs()->register_at(target_register)->reset(source->get()->copy());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opcapturecopy(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::internals::types::register_index target_register = 0;
    tie(addr, target_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Type* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    auto target_closure = static_cast<viua::types::Closure*>(target->get());
    if (target_register >= target_closure->rs()->size()) {
        throw new viua::types::Exception("cannot capture object: register index out exceeded size of closure register set");
    }

    target_closure->rs()->register_at(target_register)->reset(source->copy());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opcapturemove(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::internals::types::register_index target_register = 0;
    tie(addr, target_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::kernel::Register* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    auto target_closure = static_cast<viua::types::Closure*>(target->get());
    if (target_register >= target_closure->rs()->size()) {
        throw new viua::types::Exception("cannot capture object: register index out exceeded size of closure register set");
    }

    target_closure->rs()->register_at(target_register)->reset(source->give());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opclosure(viua::internals::types::byte* addr) {
    /** Create a closure from a function.
     */
    if (currently_used_register_set != stack.back()->local_register_set.get()) {
        throw new viua::types::Exception("creating closures from nonlocal registers is forbidden");
    }

    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    string function_name;
    tie(addr, function_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    unique_ptr<viua::kernel::RegisterSet> rs {new viua::kernel::RegisterSet(currently_used_register_set->size())};
    unique_ptr<viua::types::Closure> closure {new viua::types::Closure(function_name, std::move(rs))};

    *target = std::move(closure);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opfunction(viua::internals::types::byte* addr) {
    /** Create function object in a register.
     *
     *  Such objects can be used to call functions, and
     *  are can be used to pass functions as parameters and
     *  return them from other functions.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    string function_name;
    tie(addr, function_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Function(function_name)};

    return addr;
}

viua::internals::types::byte* viua::process::Process::opfcall(viua::internals::types::byte* addr) {
    /*  Call a function object.
     */
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* return_register = nullptr;

    if (not return_void) {
        tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Type* fn_source = nullptr;
    tie(addr, fn_source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    auto fn = dynamic_cast<viua::types::Function*>(fn_source);
    if (not fn) {
        throw new viua::types::Exception("type is not callable: " + fn_source->type());
    }

    string call_name = fn->name();

    if (not scheduler->isNativeFunction(call_name)) {
        throw new viua::types::Exception("fcall to undefined function: " + call_name);
    }

    viua::internals::types::byte* call_address = nullptr;
    call_address = adjustJumpBaseFor(call_name);

    // save return address for frame
    viua::internals::types::byte* return_address = addr;

    if (stack.frame_new == nullptr) {
        throw new viua::types::Exception("fcall without a frame: use `frame 0' in source code if the function takes no parameters");
    }

    stack.frame_new->function_name = call_name;
    stack.frame_new->return_address = return_address;
    stack.frame_new->return_register = return_register;

    if (fn->type() == "Closure") {
        stack.frame_new->setLocalRegisterSet(static_cast<viua::types::Closure*>(fn)->rs(), false);
    }

    pushFrame();

    return call_address;
}
