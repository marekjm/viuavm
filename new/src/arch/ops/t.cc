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
T::T(
    viua::arch::opcode_type const op,
    Register_access const o,
    Register_access const l,
    Register_access const r)
    : opcode{ op }
    , out{ o }
    , lhs{ l }
    , rhs{ r }
{}
auto T::decode(
    instruction_type const raw) -> T
{
    auto const opcode = carve_opcode_out(raw);
    auto const out = carve_bits_out<Register_access::underlying_type, 16>(raw);
    auto const lhs = carve_bits_out<Register_access::underlying_type, 24>(raw);
    auto const rhs = carve_bits_out<Register_access::underlying_type, 32>(raw);

    return T{ opcode,
              Register_access::decode(out),
              Register_access::decode(lhs),
              Register_access::decode(rhs) };
}
auto T::encode() const -> instruction_type
{
    return viua::compose_bits_into<instruction_type>(
        opcode,
        out.encode(),
        lhs.encode(),
        rhs.encode(),
        viua::compose_filler{ 24 });
}
auto T::to_string() const -> std::string
{
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + lhs.to_string() + ", " + rhs.to_string());
}
}  // namespace viua::arch::ops
