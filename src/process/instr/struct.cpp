/*
 *  Copyright (C) 2017-2020 Marek Marecki
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

#include <utility>

#include <viua/assert.h>
#include <viua/process.h>
#include <viua/types/atom.h>
#include <viua/types/pointer.h>
#include <viua/types/struct.h>
#include <viua/types/vector.h>


auto viua::process::Process::opstruct(Op_address_type addr) -> Op_address_type
{
    *decoder.fetch_register(addr, *this) =
        std::make_unique<viua::types::Struct>();
    return addr;
}

auto viua::process::Process::opstructinsert(Op_address_type addr)
    -> Op_address_type
{
    auto struct_operand =
        decoder.fetch_value_of<viua::types::Struct>(addr, *this);
    auto const key = decoder.fetch_value_of<viua::types::Atom>(addr, *this);

    using viua::bytecode::codec::main::get_operand_type;
    if (get_operand_type(addr) == OT_POINTER) {
        struct_operand->insert(*key, decoder.fetch_value(addr, *this)->copy());
    } else {
        struct_operand->insert(*key,
                               decoder.fetch_register(addr, *this)->give());
    }

    return addr;
}

auto viua::process::Process::opstructremove(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register_or_void(addr, *this);
    auto struct_operand =
        decoder.fetch_value_of<viua::types::Struct>(addr, *this);
    auto const key = decoder.fetch_value_of<viua::types::Atom>(addr, *this);

    auto result = struct_operand->remove(*key);
    if (target.has_value()) {
        **target = std::move(result);
    }

    return addr;
}

auto viua::process::Process::opstructat(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register_or_void(addr, *this);
    auto struct_operand =
        decoder.fetch_value_of<viua::types::Struct>(addr, *this);
    auto const key = decoder.fetch_value_of<viua::types::Atom>(addr, *this);

    if (target.has_value()) {
        **target = struct_operand->at(*key)->pointer(this);
    }

    return addr;
}

auto viua::process::Process::opstructkeys(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto struct_operand =
        decoder.fetch_value_of<viua::types::Struct>(addr, *this);

    auto keys = std::make_unique<viua::types::Vector>();
    for (auto const& each : struct_operand->keys()) {
        keys->push(std::make_unique<viua::types::Atom>(each));
    }

    *target = std::move(keys);

    return addr;
}
