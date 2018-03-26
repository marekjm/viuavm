/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <viua/process.h>
#include <viua/support/string.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/text.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::optext(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string s;
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Value* o = nullptr;
        tie(addr, o) =
            viua::bytecode::decoder::operands::fetch_object(addr, this);
        s = o->str();
    } else {
        ++addr;  // for operand type
        tie(addr, s) =
            viua::bytecode::decoder::operands::fetch_primitive_string(addr,
                                                                      this);
        s = str::strdecode(s);
    }

    *target = make_unique<viua::types::Text>(s);

    return addr;
}


viua::internals::types::byte* viua::process::Process::optexteq(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text *first = nullptr, *second = nullptr;
    tie(addr, first) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);
    tie(addr, second) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    *target = make_unique<viua::types::Boolean>(*first == *second);

    return addr;
}


static auto convert_signed_integer_to_text_size_type(
    const viua::types::Text* text,
    const int64_t signed_index) -> viua::types::Text::size_type {
    /*
     *  Cast jugglery to satisfy Clang++ with -Wsign-conversion enabled.
     */
    viua::types::Text::size_type index = 0;
    if (signed_index < 0) {
        index = (text->size() -
                 static_cast<viua::types::Text::size_type>(-signed_index));
    } else {
        index = static_cast<viua::types::Text::size_type>(signed_index);
    }
    return index;
}

viua::internals::types::byte* viua::process::Process::optextat(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text* source_text = nullptr;
    tie(addr, source_text) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    viua::types::Integer* index = nullptr;
    tie(addr, index) = viua::bytecode::decoder::operands::fetch_object_of<
        viua::types::Integer>(addr, this);

    auto working_index = convert_signed_integer_to_text_size_type(
        source_text, index->as_integer());

    *target = make_unique<viua::types::Text>(source_text->at(working_index));

    return addr;
}


viua::internals::types::byte* viua::process::Process::optextsub(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    viua::types::Integer *first_index = nullptr, *last_index = nullptr;
    tie(addr, first_index) = viua::bytecode::decoder::operands::fetch_object_of<
        viua::types::Integer>(addr, this);
    tie(addr, last_index) = viua::bytecode::decoder::operands::fetch_object_of<
        viua::types::Integer>(addr, this);

    auto working_first_index = convert_signed_integer_to_text_size_type(
        source, first_index->as_integer());
    auto working_last_index = convert_signed_integer_to_text_size_type(
        source, last_index->as_integer());

    *target = make_unique<viua::types::Text>(
        source->sub(working_first_index, working_last_index));

    return addr;
}


viua::internals::types::byte* viua::process::Process::optextlength(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    *target = make_unique<viua::types::Integer>(source->signed_size());

    return addr;
}


viua::internals::types::byte* viua::process::Process::optextcommonprefix(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text* lhs = nullptr;
    tie(addr, lhs) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    viua::types::Text* rhs = nullptr;
    tie(addr, rhs) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    *target = make_unique<viua::types::Integer>(
        static_cast<int64_t>(lhs->common_prefix(*rhs)));

    return addr;
}


viua::internals::types::byte* viua::process::Process::optextcommonsuffix(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text* lhs = nullptr;
    tie(addr, lhs) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    viua::types::Text* rhs = nullptr;
    tie(addr, rhs) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    *target = make_unique<viua::types::Integer>(
        static_cast<int64_t>(lhs->common_suffix(*rhs)));

    return addr;
}


viua::internals::types::byte* viua::process::Process::optextconcat(
    viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Text* lhs = nullptr;
    tie(addr, lhs) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    viua::types::Text* rhs = nullptr;
    tie(addr, rhs) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Text>(
            addr, this);

    *target = make_unique<viua::types::Text>((*lhs) + (*rhs));

    return addr;
}
