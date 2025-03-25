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
E::E(viua::arch::opcode_type const op,
     Register_access const o,
     uint64_t const i)
        : opcode{op}, out{o}, immediate{i}
{}
auto E::decode(instruction_type const raw) -> E
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto const out  = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto const high = (((raw >> 28) & 0xf) << 32);
    auto const low  = ((raw >> 32) & 0x00000000ffffffff);
    auto const value = (high | low);
    return E{opcode, out, value};
}
auto E::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto high            = ((immediate & 0x0000000f00000000) >> 32);
    auto low             = (immediate & 0x00000000ffffffff);
    return base | (output_register << 16) | (high << 28) | (low << 32);
}
auto E::to_string() const -> std::string
{
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + std::to_string(immediate));
}
}  // namespace viua::arch::ops
