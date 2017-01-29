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

#ifndef VIUA_BYTECODE_DECODER_OPERANDS_H
#define VIUA_BYTECODE_DECODER_OPERANDS_H

#pragma once

#include <string>
#include <utility>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/operand_types.h>
#include <viua/types/type.h>
#include <viua/kernel/registerset.h>


namespace viua {
    namespace process {
        class Process;
    }
}


namespace viua {
    namespace bytecode {
        namespace decoder {
            namespace operands {
                auto get_operand_type(const viua::internals::types::byte*) -> OperandType;

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
                auto is_void(const viua::internals::types::byte*) -> bool;
                auto fetch_void(viua::internals::types::byte*) -> viua::internals::types::byte*;
                auto fetch_operand_type(viua::internals::types::byte*) -> std::tuple<viua::internals::types::byte*, OperandType>;
                auto fetch_register_index(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::types::register_index>;
                auto fetch_register(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::kernel::Register*>;
                auto fetch_register_type_and_index(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::RegisterSets, viua::internals::types::register_index>;
                auto fetch_timeout(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::types::timeout>;
                auto fetch_registerset_type(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::types::registerset_type_marker>;
                auto fetch_primitive_uint(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::types::register_index>;
                auto fetch_primitive_uint64(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, uint64_t>;
                auto fetch_primitive_int(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::types::plain_int>;
                auto fetch_primitive_string(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, std::string>;
                auto fetch_atom(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, std::string>;
                auto fetch_object(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::types::Type*>;
                auto fetch_object2(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::types::Type*>;

                /*
                 *  Fetch raw data decoding it directly from bytecode.
                 *  No register access is neccessary.
                 *  These functions are used by instructions whose operands are always
                 *  immediates.
                 */
                auto fetch_raw_int(viua::internals::types::byte *ip, viua::process::Process* p) -> std::tuple<viua::internals::types::byte*, viua::internals::types::plain_int>;
                auto fetch_raw_float(viua::internals::types::byte*, viua::process::Process*) -> std::tuple<viua::internals::types::byte*, viua::internals::types::plain_float>;

                /*
                 *  Extract data decoding it from bytecode without advancing the bytecode
                 *  pointer.
                 *  These functions are used by instructions whose operands are always
                 *  immediates.
                 */
                auto extract_primitive_uint64(viua::internals::types::byte*, viua::process::Process*) -> uint64_t;
            }
        }
    }
}


#endif
