/*
 *  Copyright (C) 2019, 2020 Marek Marecki
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

#ifndef VIUA_BYTECODE_CODEC_MAIN_H
#define VIUA_BYTECODE_CODEC_MAIN_H

#include <cstdint>
#include <string>
#include <vector>

#include <viua/bytecode/codec.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>

namespace viua { namespace bytecode { namespace codec { namespace main {
auto get_operand_type(uint8_t const* const) -> OperandType;
auto is_void(uint8_t const* const) -> bool;

struct Decoder {
    auto decode_opcode(uint8_t const*) const
        -> std::pair<uint8_t const*, OPCODE>;

    auto decode_register(uint8_t const*) const
        -> std::pair<uint8_t const*, Register_access>;

    auto decode_string(uint8_t const*) const
        -> std::pair<uint8_t const*, std::string>;
    auto decode_bits_string(uint8_t const*) const
        -> std::pair<uint8_t const*, std::vector<uint8_t>>;
    auto decode_timeout(uint8_t const*) const
        -> std::pair<uint8_t const*, timeout_type>;

    auto decode_f64(uint8_t const*) const -> std::pair<uint8_t const*, double>;
    auto decode_i32(uint8_t const*) const -> std::pair<uint8_t const*, int32_t>;
    auto decode_bool(uint8_t const*) const -> std::pair<uint8_t const*, bool>;

    auto decode_address(uint8_t const*) const
        -> std::pair<uint8_t const*, uint64_t>;
};

struct Encoder {
    auto encode_opcode(uint8_t*, OPCODE const) const -> uint8_t*;

    auto encode_register(uint8_t*, Register_access const) const -> uint8_t*;
    auto encode_void(uint8_t*) const -> uint8_t*;

    auto encode_string(uint8_t*, std::string const) const -> uint8_t*;
    auto encode_raw_string(uint8_t*, std::string const) const -> uint8_t*;
    auto encode_bits_string(uint8_t*, std::vector<uint8_t> const) const
        -> uint8_t*;
    auto encode_timeout(uint8_t*, timeout_type const) const -> uint8_t*;

    auto encode_f64(uint8_t*, double const) const -> uint8_t*;
    auto encode_i32(uint8_t*, int32_t const) const -> uint8_t*;
    auto encode_bool(uint8_t*, bool const) const -> uint8_t*;

    auto encode_address(uint8_t*, uint64_t const) const -> uint8_t*;
};
}}}}  // namespace viua::bytecode::codec::main

#endif
