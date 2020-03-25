/*
 *  Copyright (C) 2015-2018, 2020 Marek Marecki
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
#include <viua/assert.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/kernel.h>
#include <viua/kernel/registerset.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>

using Register_index = viua::internals::types::register_index;

auto viua::process::Process::opvector(Op_address_type addr) -> Op_address_type
{
    // FIXME BRAINDAMAGE Use a different instruction to ctor an empty vector and
    // to pack a vector. The easy case is unnecessarily complicated here.

    auto const [target_rs, target_ri] =
        decoder.fetch_tagged_register_index(addr);

    auto const [pack_start_rs, pack_start_ri] =
        decoder.fetch_tagged_register_index(addr);

    auto const pack_size = decoder.fetch_register_index(addr);

    if ((target_ri > pack_start_ri)
        and (target_ri < (pack_start_ri + pack_size))) {
        // FIXME vector is inserted into a register after packing, so this
        // exception is not entirely well thought-out. Allow packing target
        // register
        throw std::make_unique<viua::types::Exception>(
            "vector would pack itself");
    }
    if ((pack_start_ri + pack_size)
        >= stack->back()->local_register_set->size()) {
        throw std::make_unique<viua::types::Exception>(
            "vector: packing outside of register set range");
    }

    using viua::bytecode::codec::register_index_type;
    for (auto i = register_index_type{0}; i < pack_size; ++i) {
        if (register_at(static_cast<register_index_type>(pack_start_ri + i), pack_start_rs)->empty()) {
            throw std::make_unique<viua::types::Exception>(
                "vector: cannot pack null register");
        }
    }

    // Check the pack_size, because if it's zero then it doesn't matter what
    // register set is used because there will be no packing.
    if (pack_size
        and (pack_start_rs != viua::bytecode::codec::Register_set::Local)) {
        throw std::make_unique<viua::types::Exception>(
            "packing vector from non-local register set is not allowed: "
            + std::to_string(static_cast<uint64_t>(pack_start_rs)) + " "
            + std::to_string(pack_size));
    }

    auto v = std::make_unique<viua::types::Vector>();
    for (auto i = register_index_type{0}; i < pack_size; ++i) {
        v->push(register_at(static_cast<register_index_type>(pack_start_ri + i), pack_start_rs)->give());
    }

    *register_at(target_ri, target_rs) = std::move(v);

    return addr;
}

auto viua::process::Process::opvinsert(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_value_of<viua::types::Vector>(addr, *this);

    using viua::bytecode::codec::main::get_operand_type;
    auto object = ((get_operand_type(addr) == OT_POINTER)
                       ? decoder.fetch_value(addr, *this)->copy()
                       : decoder.fetch_register(addr, *this)->give());

    auto index =
        decoder.fetch_value_of_or_void<viua::types::Integer>(addr, *this);
    auto position_operand_index =
        (index.has_value() ? (*index)->as_integer() : int64_t{0});

    target->insert(position_operand_index, std::move(object));

    return addr;
}

auto viua::process::Process::opvpush(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_value_of<viua::types::Vector>(addr, *this);

    using viua::bytecode::codec::main::get_operand_type;
    auto object = ((get_operand_type(addr) == OT_POINTER)
                       ? decoder.fetch_value(addr, *this)->copy()
                       : decoder.fetch_register(addr, *this)->give());

    target->push(std::move(object));

    return addr;
}

auto viua::process::Process::opvpop(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register_or_void(addr, *this);
    auto vec    = decoder.fetch_value_of<viua::types::Vector>(addr, *this);

    auto const index =
        decoder.fetch_value_of_or_void<viua::types::Integer>(addr, *this);
    auto const position_operand_index =
        (index.has_value() ? (*index)->as_integer() : int64_t{-1});

    auto ptr = vec->pop(position_operand_index);

    if (target.has_value()) {
        **target = std::move(ptr);
    }

    return addr;
}

auto viua::process::Process::opvat(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto vec    = decoder.fetch_value_of<viua::types::Vector>(addr, *this);
    auto index  = decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    *target = vec->at(index->as_integer())->pointer(this);

    return addr;
}

auto viua::process::Process::opvlen(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto vec    = decoder.fetch_value_of<viua::types::Vector>(addr, *this);

    *target = std::make_unique<viua::types::Integer>(vec->len());

    return addr;
}
