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
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/kernel/registerset.h>
#include <viua/scheduler/vps.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
#include <viua/types/value.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opcapture(
    viua::internals::types::byte* addr) {
    viua::types::Closure* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<
        viua::types::Closure>(addr, this);

    viua::internals::types::register_index target_register = 0;
    tie(addr, target_register) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::kernel::Register* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    if (target_register >= target->rs()->size()) {
        throw make_unique<viua::types::Exception>(
            "cannot capture object: register index out exceeded size of "
            "closure register set");
    }

    auto captured_object = source->get();
    auto rf = dynamic_cast<viua::types::Reference*>(captured_object);
    if (rf == nullptr) {
        /*
         * Turn captured object into a reference to take it out of VM's default
         * memory management scheme, and put it under reference-counting scheme.
         * This is needed to bind the captured object's life to lifetime of the
         * closure, instead of to lifetime of the frame.
         */
        auto ref = make_unique<viua::types::Reference>(nullptr);
        ref->rebind(source->give());

        *source = std::move(ref);  // set the register to contain the
                                   // newly-created reference
    }
    target->rs()->register_at(target_register)->reset(source->get()->copy());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opcapturecopy(
    viua::internals::types::byte* addr) {
    viua::types::Closure* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<
        viua::types::Closure>(addr, this);

    viua::internals::types::register_index target_register = 0;
    tie(addr, target_register) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Value* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object(addr, this);

    if (target_register >= target->rs()->size()) {
        throw make_unique<viua::types::Exception>(
            "cannot capture object: register index out exceeded size of "
            "closure register set");
    }

    target->rs()->register_at(target_register)->reset(source->copy());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opcapturemove(
    viua::internals::types::byte* addr) {
    viua::types::Closure* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<
        viua::types::Closure>(addr, this);

    viua::internals::types::register_index target_register = 0;
    tie(addr, target_register) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::kernel::Register* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    if (target_register >= target->rs()->size()) {
        throw make_unique<viua::types::Exception>(
            "cannot capture object: register index out exceeded size of "
            "closure register set");
    }

    target->rs()->register_at(target_register)->reset(source->give());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opclosure(
    viua::internals::types::byte* addr) {
    /** Create a closure from a function.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string function_name;
    tie(addr, function_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    auto rs = make_unique<viua::kernel::RegisterSet>(
        currently_used_register_set->size());
    auto closure =
        make_unique<viua::types::Closure>(function_name, std::move(rs));

    *target = std::move(closure);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opfunction(
    viua::internals::types::byte* addr) {
    /** Create function object in a register.
     *
     *  Such objects can be used to call functions, and
     *  are can be used to pass functions as parameters and
     *  return them from other functions.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string function_name;
    tie(addr, function_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    *target = make_unique<viua::types::Function>(function_name);

    return addr;
}
