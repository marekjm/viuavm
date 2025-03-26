/*
 *  Copyright (C) 2021-2025 Marek Marecki
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

#include <endian.h>
#include <stdint.h>
#include <string.h>

#include <string>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


namespace viua::arch::ops {
R::R(viua::arch::opcode_type const op,
     Register_access const o,
     Register_access const i,
     uint32_t const im)
        : opcode{op}, out{o}, in{i}, immediate{im}
{}
auto R::decode(instruction_type const raw) -> R
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto const out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto const in  = Register_access::decode((raw & 0x0000ffff00000000) >> 32);

    auto const low_short =
        static_cast<uint32_t>((raw & 0xffff000000000000) >> 48);
    auto const low_nibble =
        static_cast<uint32_t>((raw & 0x0000f00000000000) >> 44);
    auto const high_nibble =
        static_cast<uint32_t>((raw & 0x00000000f0000000) >> 28);

    auto const immediate = low_short | (low_nibble << 16) | (high_nibble << 20);

    return R{opcode, out, in, immediate};
}
auto R::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto input_register  = uint64_t{in.encode()};

    auto const high_nibble = uint64_t{(immediate & 0x00f00000) >> 20};
    auto const low_nibble  = uint64_t{(immediate & 0x000f0000) >> 16};
    auto const low_short   = uint64_t{(immediate & 0x0000ffff) >> 0};

    return base | (output_register << 16) | (input_register << 32)
           | (low_short << 48) | (high_nibble << 28) | (low_nibble << 44);
}
auto R::to_string() const -> std::string
{
    auto imm_str = std::to_string(immediate);
    if (not(opcode & viua::arch::ops::UNSIGNED)) {
        auto tmp = int32_t{};
        memcpy(&tmp, &immediate, sizeof(immediate));
        tmp     = ((tmp << 8) >> 8);  // sign extend
        imm_str = std::to_string(tmp);
    }
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + in.to_string() + ", " + imm_str);
}
}  // namespace viua::arch::ops
