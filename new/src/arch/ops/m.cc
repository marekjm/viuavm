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

#include <stdexcept>
#include <string>
#include <string_view>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


namespace viua::arch::ops {
M::M(viua::arch::opcode_type const op,
     Register_access const o,
     Register_access const i,
     uint16_t const im,
     uint8_t const s)
        : opcode{op}, out{o}, in{i}, immediate{im}, spec{s}
{}
auto M::decode(instruction_type const raw) -> M
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto const out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto const in  = Register_access::decode((raw & 0x0000ffff00000000) >> 32);

    auto const low_short =
        static_cast<uint16_t>((raw & 0xffff000000000000) >> 48);
    auto const low_nibble =
        static_cast<uint16_t>((raw & 0x0000f00000000000) >> 44);
    auto const high_nibble =
        static_cast<uint16_t>((raw & 0x00000000f0000000) >> 28);

    auto const immediate = low_short;
    auto const spec = static_cast<uint8_t>(low_nibble | (high_nibble << 4));

    return M{opcode, out, in, immediate, spec};
}
auto M::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto input_register  = uint64_t{in.encode()};

    auto const high_nibble = static_cast<uint64_t>(spec & 0xf0);
    auto const low_nibble  = static_cast<uint64_t>(spec & 0x0f);
    auto const low_short = static_cast<uint64_t>((immediate & 0x0000ffff) >> 0);

    return base | (output_register << 16) | (input_register << 32)
           | (low_short << 48) | (high_nibble << 28) | (low_nibble << 44);
}
auto M::to_string() const -> std::string
{
    return (viua::arch::ops::to_string(opcode) + " "
            + std::to_string(static_cast<uintmax_t>(spec)) + ", "
            + out.to_string() + ", " + in.to_string() + ", "
            + std::to_string(static_cast<uintmax_t>(immediate)));
}
}  // namespace viua::arch::ops
