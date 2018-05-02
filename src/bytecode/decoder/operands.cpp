/*
 *  Copyright (C) 2016, 2017, 2018 Marek Marecki
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
#include <utility>
#include <viua/bytecode/decoder/operands.h>
#include <viua/bytecode/operand_types.h>
#include <viua/exceptions.h>
#include <viua/process.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/types/reference.h>
#include <viua/types/value.h>
#include <viua/util/memory.h>
using namespace std;

using viua::process::Op_address_type;
using viua::util::memory::aligned_read;
using viua::util::memory::load_aligned;


template<class T>
static auto extract(Op_address_type ip) -> std::remove_const_t<T> {
    std::remove_const_t<T> data{};
    std::memcpy(&data, ip, sizeof(T));
    return data;
}
template<class T> static auto extract_ptr(Op_address_type ip) -> T* {
    return reinterpret_cast<T*>(ip);
}

auto viua::bytecode::decoder::operands::get_operand_type(
    viua::internals::types::byte const* const ip) -> OperandType {
    return extract<OperandType const>(ip);
}

auto viua::bytecode::decoder::operands::is_void(
    viua::internals::types::byte const* const ip) -> bool {
    return (get_operand_type(ip) == OT_VOID);
}

auto viua::bytecode::decoder::operands::fetch_void(Op_address_type ip)
    -> Op_address_type {
    auto const ot = get_operand_type(ip);
    ++ip;

    if (ot != OT_VOID) {
        throw make_unique<viua::types::Exception>(
            "decoded invalid operand type: expected OT_VOID");
    }

    return ip;
}

auto viua::bytecode::decoder::operands::fetch_operand_type(Op_address_type ip)
    -> tuple<Op_address_type, OperandType> {
    auto const ot = get_operand_type(ip);
    ++ip;
    return tuple<Op_address_type, OperandType>{ip, ot};
}

static auto integer_to_register_index(
    viua::types::Integer::underlying_type const n)
    -> viua::internals::types::register_index {
    // FIXME maximum integer is greater than maximum register index, add bouns
    // checking
    return static_cast<viua::internals::types::register_index>(n);
}
static auto extract_register_index(Op_address_type ip,
                                   viua::process::Process* process,
                                   bool pointers_allowed = false)
    -> tuple<Op_address_type, viua::internals::types::register_index> {
    auto const ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    auto register_index = viua::internals::types::register_index{0};
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE
        or (pointers_allowed and ot == OT_POINTER)) {
        register_index = extract<viua::internals::types::register_index>(ip);
        ip += sizeof(viua::internals::types::register_index);

        // FIXME extract RS type
        ip += sizeof(viua::internals::Register_sets);
    } else {
        throw make_unique<viua::types::Exception>(
            "decoded invalid operand type: expected OT_REGISTER_INDEX, "
            "OT_REGISTER_REFERENCE"
            + (pointers_allowed ? std::string(", OT_POINTER")
                                : std::string("")));
    }
    if (ot == OT_REGISTER_REFERENCE) {
        auto i =
            static_cast<viua::types::Integer*>(process->obtain(register_index));
        // FIXME Number::negative() -> bool is needed
        if (i->as_integer() < 0) {
            throw make_unique<viua::types::Exception>(
                "register indexes cannot be negative");
        }
        register_index = integer_to_register_index(i->as_integer());
    }
    return tuple<Op_address_type, viua::internals::types::register_index>(
        ip, register_index);
}
static auto extract_register_type_and_index(Op_address_type ip,
                                            viua::process::Process* process,
                                            bool const pointers_allowed = false)
    -> tuple<Op_address_type,
             viua::internals::Register_sets,
             viua::internals::types::register_index> {
    auto const ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    auto register_type  = viua::internals::Register_sets::LOCAL;
    auto register_index = viua::internals::types::register_index{0};
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE
        or (pointers_allowed and ot == OT_POINTER)) {
        register_index = extract<viua::internals::types::register_index>(ip);
        ip += sizeof(viua::internals::types::register_index);

        register_type = extract<viua::internals::Register_sets>(ip);
        ip += sizeof(viua::internals::Register_sets);
    } else {
        throw make_unique<viua::types::Exception>(
            "decoded invalid operand type: expected OT_REGISTER_INDEX, "
            "OT_REGISTER_REFERENCE"
            + (pointers_allowed ? std::string(", OT_POINTER")
                                : std::string("")));
    }
    if (ot == OT_REGISTER_REFERENCE) {
        auto const i =
            static_cast<viua::types::Integer*>(process->obtain(register_index));
        // FIXME Number::negative() -> bool is needed
        if (i->as_integer() < 0) {
            throw make_unique<viua::types::Exception>(
                "register indexes cannot be negative");
        }
        register_index = integer_to_register_index(i->as_integer());
    }
    return tuple<Op_address_type,
                 viua::internals::Register_sets,
                 viua::internals::types::register_index>(
        ip, register_type, register_index);
}
auto viua::bytecode::decoder::operands::fetch_register_index(
    Op_address_type ip,
    viua::process::Process* process)
    -> tuple<Op_address_type, viua::internals::types::register_index> {
    return extract_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_register(
    Op_address_type ip,
    viua::process::Process* process)
    -> tuple<Op_address_type, viua::kernel::Register*> {
    auto register_type = viua::internals::Register_sets::LOCAL;
    auto target        = viua::internals::types::register_index{0};
    tie(ip, register_type, target) =
        extract_register_type_and_index(ip, process);
    return tuple<Op_address_type, viua::kernel::Register*>(
        ip, process->register_at(target, register_type));
}

auto viua::bytecode::decoder::operands::fetch_register_type_and_index(
    Op_address_type ip,
    viua::process::Process* process)
    -> tuple<Op_address_type,
             viua::internals::Register_sets,
             viua::internals::types::register_index> {
    auto register_type = viua::internals::Register_sets::LOCAL;
    auto target        = viua::internals::types::register_index{0};
    tie(ip, register_type, target) =
        extract_register_type_and_index(ip, process);
    return tuple<Op_address_type,
                 viua::internals::Register_sets,
                 viua::internals::types::register_index>(
        ip, register_type, target);
}

auto viua::bytecode::decoder::operands::fetch_timeout(Op_address_type ip,
                                                      viua::process::Process*)
    -> tuple<Op_address_type, viua::internals::types::timeout> {
    auto const ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    auto value = viua::internals::types::timeout{0};
    if (ot == OT_INT) {
        aligned_read(value) = ip;
        ip += sizeof(decltype(value));
    } else {
        throw make_unique<viua::types::Exception>(
            "decoded invalid operand type: expected O_INT");
    }
    return tuple<Op_address_type, viua::internals::types::timeout>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint(
    Op_address_type ip,
    viua::process::Process* process)
    -> tuple<Op_address_type, viua::internals::types::register_index> {
    return fetch_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_registerset_type(
    Op_address_type ip,
    viua::process::Process*)
    -> tuple<Op_address_type, viua::internals::types::registerset_type_marker> {
    auto const rs_type =
        extract<viua::internals::types::registerset_type_marker>(ip);
    ip += sizeof(decltype(rs_type));
    return tuple<Op_address_type, decltype(rs_type)>(ip, rs_type);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint64(
    Op_address_type ip,
    viua::process::Process*) -> tuple<Op_address_type, uint64_t> {
    auto const integer = extract<uint64_t>(ip);
    ip += sizeof(decltype(integer));
    return tuple<Op_address_type, decltype(integer)>(ip, integer);
}

auto viua::bytecode::decoder::operands::fetch_primitive_int(
    Op_address_type ip,
    viua::process::Process* p)
    -> tuple<Op_address_type, viua::internals::types::plain_int> {
    auto const ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    auto value = viua::internals::types::plain_int{0};
    if (ot == OT_REGISTER_REFERENCE) {
        auto const index =
            *reinterpret_cast<viua::internals::types::register_index const*>(
                ip);
        ip += sizeof(viua::internals::types::register_index);

        // FIXME decode rs type
        ip += sizeof(viua::internals::Register_sets);

        // FIXME once dynamic operand types are implemented the need for this
        // cast will go away because the operand *will* be encoded as a real
        // uint
        auto const i = static_cast<viua::types::Integer*>(p->obtain(index));

        // FIXME plain_int (as encoded in bytecode) is 32 bits, but in-program
        // integer is 64 bits
        value = static_cast<decltype(value)>(i->as_integer());
    } else if (ot == OT_INT) {
        aligned_read(value) = ip;
        ip += sizeof(decltype(value));
    } else {
        throw make_unique<viua::types::Exception>(
            "decoded invalid operand type: expected OT_REGISTER_REFERENCE, "
            "OT_INT");
    }
    return tuple<Op_address_type, viua::internals::types::plain_int>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_raw_int(Op_address_type ip,
                                                      viua::process::Process*)
    -> tuple<Op_address_type, viua::internals::types::plain_int> {
    return tuple<Op_address_type, viua::internals::types::plain_int>(
        (ip + sizeof(viua::internals::types::plain_int)),
        extract<viua::internals::types::plain_int>(ip));
}

auto viua::bytecode::decoder::operands::fetch_raw_float(Op_address_type ip,
                                                        viua::process::Process*)
    -> tuple<Op_address_type, viua::internals::types::plain_float> {
    return tuple<Op_address_type, viua::internals::types::plain_float>(
        (ip + sizeof(viua::internals::types::plain_float)),
        extract<viua::internals::types::plain_float>(ip));
}

auto viua::bytecode::decoder::operands::extract_primitive_uint64(
    Op_address_type ip,
    viua::process::Process*) -> uint64_t {
    return extract<uint64_t>(ip);
}

auto viua::bytecode::decoder::operands::fetch_primitive_string(
    Op_address_type ip,
    viua::process::Process*) -> tuple<Op_address_type, std::string> {
    auto const s = std::string{extract_ptr<char const>(ip)};
    ip += (s.size() + 1);
    return tuple<Op_address_type, std::string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_atom(Op_address_type ip,
                                                   viua::process::Process*)
    -> tuple<Op_address_type, std::string> {
    auto const s = std::string{extract_ptr<char const>(ip)};
    ip += (s.size() + 1);
    return tuple<Op_address_type, std::string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_object(Op_address_type ip,
                                                     viua::process::Process* p)
    -> tuple<Op_address_type, viua::types::Value*> {
    auto const is_pointer_dereference = (get_operand_type(ip) == OT_POINTER);

    auto register_type = viua::internals::Register_sets::LOCAL;
    auto target        = viua::internals::types::register_index{0};
    tie(ip, register_type, target) =
        extract_register_type_and_index(ip, p, true);

    auto object = p->register_at(target, register_type)->get();
    if (object == nullptr) {
        ostringstream oss;
        oss << "read from null register: " << target;
        throw make_unique<viua::types::Exception>(oss.str());
    }

    if (auto ref = dynamic_cast<viua::types::Reference*>(object)) {
        object = ref->points_to();
    }

    if (is_pointer_dereference) {
        auto const pointer_object = dynamic_cast<viua::types::Pointer*>(object);
        if (pointer_object == nullptr) {
            throw make_unique<viua::types::Exception>(
                "dereferenced type is not a pointer: " + object->type());
        }
        object = pointer_object->to(p);
    }
    if (auto pointer_object = dynamic_cast<viua::types::Pointer*>(object)) {
        pointer_object->authenticate(p);
    }

    return tuple<Op_address_type, viua::types::Value*>(ip, object);
}
