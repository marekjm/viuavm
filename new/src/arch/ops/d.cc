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

#include <stdint.h>

#include <string>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


namespace viua::arch::ops {
D::D(
    viua::arch::opcode_type const op,
    Register_access const o,
    Register_access const i)
    : opcode{ op }
    , out{ o }
    , in{ i }
{}
auto D::decode(
    instruction_type const raw) -> D
{
    auto opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x00'00'00'00'00'00'ff'ff);
    auto out = Register_access::decode((raw & 0x00'00'00'00'ff'ff'00'00) >> 16);
    auto in  = Register_access::decode((raw & 0x00'00'ff'ff'00'00'00'00) >> 32);
    return D{ opcode, out, in };
}
auto D::encode() const -> instruction_type
{
    auto base            = uint64_t{ opcode };
    auto output_register = uint64_t{ out.encode() };
    auto input_register  = uint64_t{ in.encode() };
    return base | (output_register << 16) | (input_register << 32);
}
auto D::to_string() const -> std::string
{
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + in.to_string());
}
}  // namespace viua::arch::ops
