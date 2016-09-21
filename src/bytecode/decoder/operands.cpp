/*
 *  Copyright (C) 2016 Marek Marecki
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
#include <viua/bytecode/operand_types.h>
#include <viua/types/type.h>
#include <viua/types/byte.h>
#include <viua/types/integer.h>
#include <viua/process.h>
#include <viua/exceptions.h>
#include <viua/bytecode/decoder/operands.h>
using namespace std;


template<class T> static auto extract(byte *ip) -> T {
    return *reinterpret_cast<T*>(ip);
}
template<class T> static auto extract_ptr(byte *ip) -> T* {
    return reinterpret_cast<T*>(ip);
}

static auto get_operand_type(byte *ip) -> OperandType {
    return extract<OperandType>(ip);
}

auto viua::bytecode::decoder::operands::fetch_register_index(byte *ip, Process *process) -> tuple<byte*, unsigned> {
    OperandType ot = get_operand_type(ip);
    ++ip;

    unsigned register_index = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE) {
        // FIXME currently RI's are encoded as signed integers
        // remove this ugly cast when this is fixed
        register_index = static_cast<unsigned>(extract<int>(ip));
        ip += sizeof(int);
    } else {
        throw new Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        Integer *i = static_cast<Integer*>(process->obtain(register_index));
        if (i->as_integer() < 0) {
            throw new Exception("register indexes cannot be negative");
        }
        register_index = static_cast<unsigned>(i->as_integer());
    }
    return tuple<byte*, unsigned>(ip, register_index);
}

auto viua::bytecode::decoder::operands::fetch_primitive_char(byte *ip, Process *process) -> tuple<byte*, char> {
    OperandType ot = get_operand_type(ip);
    ++ip;

    char value = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE) {
        value = extract<char>(ip);
        ip += sizeof(char);
    } else {
        throw new Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        // FIXME once dynamic operand types are implemented the need for this cast will go away
        // because the operand *will* be encoded as a real uint
        Byte *i = static_cast<Byte*>(process->obtain(static_cast<unsigned>(value)));
        value = i->value();
    }
    return tuple<byte*, char>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint(byte *ip, Process *process) -> tuple<byte*, unsigned> {
    // currently the logic is the same since RI's are encoded as unsigned integers
    return fetch_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint64(byte *ip, Process*) -> tuple<byte*, uint64_t> {
    uint64_t integer = extract<decltype(integer)>(ip);
    ip += sizeof(decltype(integer));
    return tuple<byte*, decltype(integer)>(ip, integer);
}

auto viua::bytecode::decoder::operands::fetch_primitive_int(byte *ip, Process* p) -> tuple<byte*, int> {
    OperandType ot = get_operand_type(ip);
    ++ip;

    int value = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE) {
        value = extract<int>(ip);
        ip += sizeof(int);
    } else {
        throw new Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        // FIXME once dynamic operand types are implemented the need for this cast will go away
        // because the operand *will* be encoded as a real uint
        Integer *i = static_cast<Integer*>(p->obtain(static_cast<unsigned>(value)));
        value = i->value();
    }
    return tuple<byte*, int>(ip, value);
}

auto viua::bytecode::decoder::operands::extract_primitive_uint64(byte *ip, Process*) -> uint64_t {
    return extract<uint64_t>(ip);
}

auto viua::bytecode::decoder::operands::fetch_atom(byte *ip, Process*) -> tuple<byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<byte*, string>(ip, s);
}
