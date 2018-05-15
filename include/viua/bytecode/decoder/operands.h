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

#ifndef VIUA_BYTECODE_DECODER_OPERANDS_H
#define VIUA_BYTECODE_DECODER_OPERANDS_H

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/operand_types.h>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/value.h>
#include <viua/util/memory.h>


namespace viua { namespace process {
class Process;
}}  // namespace viua::process


namespace viua { namespace bytecode { namespace decoder { namespace operands {

using viua::internals::types::Op_address_type;

auto get_operand_type(Op_address_type const) -> OperandType;

/*
 *  Fetch fully specified operands, possibly stored in registers.
 *  These functions will correctly decode and return:
 *
 *  - immediates
 *  - values from registers specified as register indexes
 *  - values from registers specified as register references
 *
 *  These functions are used most often by majority of the instructions.
 *
 */
auto is_void(Op_address_type const) -> bool;
auto fetch_void(Op_address_type) -> Op_address_type;
auto fetch_operand_type(Op_address_type)
    -> std::tuple<Op_address_type, OperandType>;
auto fetch_register_index(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::internals::types::register_index>;
auto fetch_register(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::kernel::Register*>;
auto fetch_register_type_and_index(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type,
                  viua::internals::Register_sets,
                  viua::internals::types::register_index>;
auto fetch_timeout(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::internals::types::timeout>;
auto fetch_registerset_type(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type,
                  viua::internals::types::registerset_type_marker>;
auto fetch_primitive_uint(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::internals::types::register_index>;
auto fetch_primitive_uint64(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, uint64_t>;
auto fetch_primitive_int(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::internals::types::plain_int>;
auto fetch_primitive_string(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, std::string>;
auto fetch_atom(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, std::string>;
auto fetch_object(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::types::Value*>;

template<typename RequestedType>
auto fetch_object_of(Op_address_type ip, viua::process::Process* p)
    -> std::tuple<Op_address_type, RequestedType*> {
    auto [addr_, fetched] = fetch_object(ip, p);

    RequestedType* converted = dynamic_cast<RequestedType*>(fetched);
    if (not converted) {
        throw std::make_unique<viua::types::Exception>(
            "fetched invalid type: expected '" + RequestedType::type_name
            + "' but got '" + fetched->type() + "'");
    }
    return {addr_, converted};
}

/*
 *  Fetch raw data decoding it directly from bytecode.
 *  No register access is neccessary.
 *  These functions are used by instructions whose operands are always
 *  immediates.
 */
auto fetch_raw_int(Op_address_type ip, viua::process::Process* p)
    -> std::tuple<Op_address_type, viua::internals::types::plain_int>;
auto fetch_raw_float(Op_address_type, viua::process::Process*)
    -> std::tuple<Op_address_type, viua::internals::types::plain_float>;

/*
 *  Extract data decoding it from bytecode without advancing the bytecode
 *  pointer.
 *  These functions are used by instructions whose operands are always
 *  immediates.
 */
auto extract_primitive_uint64(Op_address_type, viua::process::Process*)
    -> uint64_t;
}}}}  // namespace viua::bytecode::decoder::operands


#endif
