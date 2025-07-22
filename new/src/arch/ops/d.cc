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
    auto const opcode = carve_opcode_out(raw);
    auto const dst = carve_bits_out<Register_access::underlying_type, 8>(raw);
    auto const src = carve_bits_out<Register_access::underlying_type, 0>(raw);

    return D{ opcode,
              Register_access::decode(dst),
              Register_access::decode(src) };
}
auto D::encode() const -> instruction_type
{
    return viua::compose_bits_into<instruction_type>(
        in.encode(), out.encode(), viua::compose_filler{ 32 }, opcode);
}
auto D::to_string() const -> std::string
{
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + in.to_string());
}
}  // namespace viua::arch::ops
