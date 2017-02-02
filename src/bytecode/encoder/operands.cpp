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
using namespace std;


template<class T> static auto extract(viua::internals::types::byte *ip) -> T {
    return *reinterpret_cast<T*>(ip);
}
template<class T> static auto extract_ptr(viua::internals::types::byte *ip) -> T* {
    return reinterpret_cast<T*>(ip);
}

static auto get_operand_type(viua::internals::types::byte *ip) -> OperandType {
    return extract<OperandType>(ip);
}

template<class Value, class Class> static auto fetch_primitive_value(viua::internals::types::byte *ip, viua::process::Process *p, Value initial) -> tuple<viua::internals::types::byte*, Value> {
    OperandType ot = get_operand_type(ip);
    ++ip;

    Value value = initial;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE) {
        value = extract<Value>(ip);
        ip += sizeof(Value);
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        // FIXME once dynamic operand types are implemented the need for this cast will go away
        // because the operand *will* be encoded as a real uint
        Class *i = static_cast<Class*>(p->obtain(static_cast<unsigned>(value)));
        value = i->value();
    }
    return tuple<viua::internals::types::byte*, Value>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_register_index(viua::internals::types::byte *ip, viua::process::Process *process) -> tuple<viua::internals::types::byte*, unsigned> {
    OperandType ot = get_operand_type(ip);
    ++ip;

    unsigned register_index = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE) {
        // FIXME currently RI's are encoded as signed integers
        // remove this ugly cast when this is fixed
        register_index = static_cast<unsigned>(extract<int>(ip));
        ip += sizeof(int);
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        Integer *i = static_cast<Integer*>(process->obtain(register_index));
        if (i->as_int32() < 0) {
            throw new viua::types::Exception("register indexes cannot be negative");
        }
        register_index = i->as_uint32();
    }
    return tuple<viua::internals::types::byte*, unsigned>(ip, register_index);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint(viua::internals::types::byte *ip, viua::process::Process *process) -> tuple<viua::internals::types::byte*, unsigned> {
    // currently the logic is the same since RI's are encoded as unsigned integers
    return fetch_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint64(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, uint64_t> {
    uint64_t integer = extract<decltype(integer)>(ip);
    ip += sizeof(decltype(integer));
    return tuple<viua::internals::types::byte*, decltype(integer)>(ip, integer);
}

auto viua::bytecode::decoder::operands::fetch_primitive_int(viua::internals::types::byte *ip, viua::process::Process* p) -> tuple<viua::internals::types::byte*, int> {
    return fetch_primitive_value<int, Integer>(ip, p, 0);
}

auto viua::bytecode::decoder::operands::fetch_raw_int(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, int> {
    return tuple<viua::internals::types::byte*, int>((ip+sizeof(int)), extract<int>(ip));
}

auto viua::bytecode::decoder::operands::fetch_raw_float(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, float> {
    return tuple<viua::internals::types::byte*, float>((ip+sizeof(float)), extract<float>(ip));
}

auto viua::bytecode::decoder::operands::extract_primitive_uint64(viua::internals::types::byte *ip, viua::process::Process*) -> uint64_t {
    return extract<uint64_t>(ip);
}

auto viua::bytecode::decoder::operands::fetch_primitive_string(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<viua::internals::types::byte*, string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_atom(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<viua::internals::types::byte*, string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_object(viua::internals::types::byte *ip, viua::process::Process *p) -> tuple<viua::internals::types::byte*, Type*> {
    unsigned register_index = 0;
    tie(ip, register_index) = fetch_register_index(ip, p);
    return tuple<viua::internals::types::byte*, Type*>(ip, p->obtain(register_index));
}
