/*
 *  Copyright (C) 2017, 2018, 2020 Marek Marecki
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
#include <viua/process.h>
#include <viua/support/string.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/text.h>


auto viua::process::Process::optext(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);

    auto ot      = viua::bytecode::codec::main::get_operand_type(addr);
    auto const s = (ot == OT_REGISTER_INDEX or ot == OT_POINTER)
                       ? decoder.fetch_value(addr, *this)->str()
                       : str::strdecode(decoder.fetch_string(++addr));

    *target = std::make_unique<viua::types::Text>(s);

    return addr;
}


auto viua::process::Process::optexteq(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto lhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);
    auto rhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);

    *target = std::make_unique<viua::types::Boolean>(*lhs == *rhs);

    return addr;
}


static auto convert_signed_integer_to_text_size_type(
    const viua::types::Text* text,
    const int64_t signed_index) -> viua::types::Text::size_type
{
    viua::types::Text::size_type index = 0;
    if (signed_index < 0) {
        index = (text->size()
                 - static_cast<viua::types::Text::size_type>(-signed_index));
    } else {
        index = static_cast<viua::types::Text::size_type>(signed_index);
    }
    return index;
}

auto viua::process::Process::optextat(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto text   = decoder.fetch_value_of<viua::types::Text>(addr, *this);
    auto index  = decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    auto working_index =
        convert_signed_integer_to_text_size_type(text, index->as_integer());

    *target = std::make_unique<viua::types::Text>(text->at(working_index));

    return addr;
}


auto viua::process::Process::optextsub(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_value_of<viua::types::Text>(addr, *this);
    auto first_index =
        decoder.fetch_value_of<viua::types::Integer>(addr, *this);
    auto last_index = decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    auto working_first_index = convert_signed_integer_to_text_size_type(
        source, first_index->as_integer());
    auto working_last_index = convert_signed_integer_to_text_size_type(
        source, last_index->as_integer());

    *target = std::make_unique<viua::types::Text>(
        source->sub(working_first_index, working_last_index));

    return addr;
}


auto viua::process::Process::optextlength(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_value_of<viua::types::Text>(addr, *this);

    *target = std::make_unique<viua::types::Integer>(source->signed_size());

    return addr;
}


auto viua::process::Process::optextcommonprefix(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto lhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);
    auto rhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);

    *target = std::make_unique<viua::types::Integer>(
        static_cast<int64_t>(lhs->common_prefix(*rhs)));

    return addr;
}


auto viua::process::Process::optextcommonsuffix(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto lhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);
    auto rhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);

    *target = std::make_unique<viua::types::Integer>(
        static_cast<int64_t>(lhs->common_suffix(*rhs)));

    return addr;
}


auto viua::process::Process::optextconcat(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto lhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);
    auto rhs    = decoder.fetch_value_of<viua::types::Text>(addr, *this);

    *target = std::make_unique<viua::types::Text>((*lhs) + (*rhs));

    return addr;
}
