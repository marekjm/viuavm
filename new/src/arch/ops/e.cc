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
E::E(
    viua::arch::opcode_type const op,
    Register_access const o,
    uint64_t const i)
    : opcode{ op }
    , out{ o }
    , immediate{ i }
{}
auto E::decode(
    instruction_type const raw) -> E
{
    auto const opcode = carve_opcode_out(raw);
    auto const out   = carve_bits_out<Register_access::underlying_type, 0>(raw);
    auto const low   = carve_bits_out<uint32_t, 8>(raw);
    auto const high  = carve_bits_out<uint8_t, 40>(raw) & 0x0f;
    auto const value = (high | low);

    return E{ opcode, Register_access::decode(out), value };
}
auto E::encode() const -> instruction_type
{
    auto base            = uint64_t{ opcode };
    auto output_register = uint64_t{ out.encode() };
    auto high            = ((immediate & 0x00'00'00'0f'00'00'00'00) >> 32);
    auto low             = (immediate & 0x00'00'00'00'ff'ff'ff'ff);

    return (base << 48) | (high << 40) | (low << 8) | output_register;
}
auto E::to_string() const -> std::string
{
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + std::to_string(immediate));
}
}  // namespace viua::arch::ops
