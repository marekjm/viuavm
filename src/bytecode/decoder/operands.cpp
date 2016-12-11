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
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/process.h>
#include <viua/exceptions.h>
#include <viua/bytecode/decoder/operands.h>
using namespace std;


template<class T> static auto extract(byte *ip) -> T {
    return *reinterpret_cast<T*>(ip);
}
template<class T> static auto extract(const byte *ip) -> T {
    return *reinterpret_cast<T*>(ip);
}
template<class T> static auto extract_ptr(byte *ip) -> T* {
    return reinterpret_cast<T*>(ip);
}

auto viua::bytecode::decoder::operands::get_operand_type(const byte *ip) -> OperandType {
    return extract<const OperandType>(ip);
}

auto viua::bytecode::decoder::operands::is_void(const byte *ip) -> bool {
    OperandType ot = get_operand_type(ip);
    return (ot == OT_VOID);
}

auto viua::bytecode::decoder::operands::fetch_void(byte *ip) -> byte* {
    OperandType ot = get_operand_type(ip);
    ++ip;

    if (ot != OT_VOID) {
        throw new viua::types::Exception("decoded invalid operand type");
    }

    return ip;
}

auto viua::bytecode::decoder::operands::fetch_operand_type(byte *ip) -> tuple<byte*, OperandType> {
    OperandType ot = get_operand_type(ip);
    ++ip;
    return tuple<byte*, OperandType>{ip, ot};
}

static auto extract_register_index(byte *ip, viua::process::Process *process, bool pointers_allowed = false) -> tuple<byte*, viua::internals::types::register_index> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    viua::internals::types::register_index register_index = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE or (pointers_allowed and ot == OT_POINTER)) {
        register_index = extract<viua::internals::types::register_index>(ip);
        ip += sizeof(viua::internals::types::register_index);
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        auto i = static_cast<viua::types::Integer*>(process->obtain(register_index));
        // FIXME Number::negative() -> bool is needed
        if (i->as_int32() < 0) {
            throw new viua::types::Exception("register indexes cannot be negative");
        }
        register_index = i->as_uint32();
    }
    return tuple<byte*, viua::internals::types::register_index>(ip, register_index);
}
auto viua::bytecode::decoder::operands::fetch_register_index(byte *ip, viua::process::Process *process) -> tuple<byte*, viua::internals::types::register_index> {
    return extract_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_timeout(byte *ip, viua::process::Process*) -> tuple<byte*, viua::internals::types::timeout> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    viua::internals::types::timeout value = 0;
    if (ot == OT_INT) {
        value = *reinterpret_cast<decltype(value)*>(ip);
        ip += sizeof(decltype(value));
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    return tuple<byte*, viua::internals::types::timeout>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint(byte *ip, viua::process::Process *process) -> tuple<byte*, viua::internals::types::register_index> {
    // currently the logic is the same since RI's are encoded as unsigned integers
    return fetch_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint64(byte *ip, viua::process::Process*) -> tuple<byte*, uint64_t> {
    uint64_t integer = extract<decltype(integer)>(ip);
    ip += sizeof(decltype(integer));
    return tuple<byte*, decltype(integer)>(ip, integer);
}

auto viua::bytecode::decoder::operands::fetch_primitive_int(byte *ip, viua::process::Process* p) -> tuple<byte*, int> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    int value = 0;
    if (ot == OT_REGISTER_REFERENCE) {
        value = *reinterpret_cast<decltype(value)*>(ip);
        ip += sizeof(decltype(value));
        // FIXME once dynamic operand types are implemented the need for this cast will go away
        // because the operand *will* be encoded as a real uint
        viua::types::Integer *i = static_cast<viua::types::Integer*>(p->obtain(static_cast<unsigned>(value)));
        value = i->value();
    } else if (ot == OT_INT) {
        value = *reinterpret_cast<decltype(value)*>(ip);
        ip += sizeof(decltype(value));
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    return tuple<byte*, int>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_raw_int(byte *ip, viua::process::Process*) -> tuple<byte*, int> {
    return tuple<byte*, int>((ip+sizeof(int)), extract<int>(ip));
}

auto viua::bytecode::decoder::operands::fetch_raw_float(byte *ip, viua::process::Process*) -> tuple<byte*, float> {
    return tuple<byte*, float>((ip+sizeof(float)), extract<float>(ip));
}

auto viua::bytecode::decoder::operands::extract_primitive_uint64(byte *ip, viua::process::Process*) -> uint64_t {
    return extract<uint64_t>(ip);
}

auto viua::bytecode::decoder::operands::fetch_primitive_string(byte *ip, viua::process::Process*) -> tuple<byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<byte*, string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_atom(byte *ip, viua::process::Process*) -> tuple<byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<byte*, string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_object(byte *ip, viua::process::Process *p) -> tuple<byte*, viua::types::Type*> {
    unsigned register_index = 0;

    bool is_pointer = (get_operand_type(ip) == OT_POINTER);

    tie(ip, register_index) = extract_register_index(ip, p, true);
    auto object = p->obtain(register_index);

    if (is_pointer) {
        auto pointer_object = dynamic_cast<viua::types::Pointer*>(object);
        if (pointer_object == nullptr) {
            throw new viua::types::Exception("dereferenced type is not a pointer: " + object->type());
        }
        object = pointer_object->to();
    }

    return tuple<byte*, viua::types::Type*>(ip, object);
}
