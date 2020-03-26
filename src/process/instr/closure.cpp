/*
 *  Copyright (C) 2015, 2016, 2018-2020 Marek Marecki
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

#include <algorithm>
#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/kernel/registerset.h>
#include <viua/scheduler/process.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
#include <viua/types/value.h>


auto viua::process::Process::opcapture(Op_address_type addr) -> Op_address_type
{
    auto const target =
        decoder.fetch_value_of<viua::types::Closure>(addr, *this);
    auto const target_register = decoder.fetch_register_index(addr);
    auto const source          = decoder.fetch_register(addr, *this);

    if (target_register >= target->rs()->size()) {
        throw std::make_unique<viua::types::Exception>(
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
        auto ref = std::make_unique<viua::types::Reference>(nullptr);
        ref->rebind(source->give());

        *source = std::move(ref);  // set the register to contain the
                                   // newly-created reference
    }
    target->rs()->register_at(target_register)->reset(source->get()->copy());

    return addr;
}

auto viua::process::Process::opcapturecopy(Op_address_type addr)
    -> Op_address_type
{
    auto const target =
        decoder.fetch_value_of<viua::types::Closure>(addr, *this);
    auto const target_register = decoder.fetch_register_index(addr);
    auto const source          = decoder.fetch_value(addr, *this);

    if (target_register >= target->rs()->size()) {
        throw std::make_unique<viua::types::Exception>(
            "cannot capture object: register index out exceeded size of "
            "closure register set");
    }

    target->rs()->register_at(target_register)->reset(source->copy());

    return addr;
}

auto viua::process::Process::opcapturemove(Op_address_type addr)
    -> Op_address_type
{
    auto const target =
        decoder.fetch_value_of<viua::types::Closure>(addr, *this);
    auto const target_register = decoder.fetch_register_index(addr);
    auto const source          = decoder.fetch_register(addr, *this);

    if (target_register >= target->rs()->size()) {
        throw std::make_unique<viua::types::Exception>(
            "cannot capture object: register index out exceeded size of "
            "closure register set");
    }

    target->rs()->register_at(target_register)->reset(source->give());

    return addr;
}

auto viua::process::Process::opclosure(Op_address_type addr) -> Op_address_type
{
    auto const target        = decoder.fetch_register(addr, *this);
    auto const function_name = decoder.fetch_string(addr);

    auto rs = std::make_unique<viua::kernel::Register_set>(
        std::max(stack->back()->local_register_set->size(),
                 viua::bytecode::codec::register_index_type{16}));
    auto closure =
        std::make_unique<viua::types::Closure>(function_name, std::move(rs));

    *target = std::move(closure);

    return addr;
}

auto viua::process::Process::opfunction(Op_address_type addr) -> Op_address_type
{
    auto const target        = decoder.fetch_register(addr, *this);
    auto const function_name = decoder.fetch_string(addr);

    *target = std::make_unique<viua::types::Function>(function_name);

    return addr;
}
