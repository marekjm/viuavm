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
F::F(
    viua::arch::opcode_type const op,
    Register_access const o,
    uint32_t const i)
    : opcode{ op }
    , out{ o }
    , immediate{ i }
{}
auto F::decode(
    instruction_type const raw) -> F
{
    auto const opcode = carve_opcode_out(raw);
    if ((opcode & viua::arch::ops::FORMAT_MASK) != viua::arch::ops::FORMAT_F) {
        throw std::runtime_error{ "F::decode: not an F format instruction" };
    }

    auto const out   = carve_bits_out<Register_access::underlying_type, 0>(raw);
    auto const value = carve_bits_out<uint32_t, 8>(raw);

    return F{ opcode, Register_access::decode(out), le32toh(value) };
}
auto F::encode() const -> instruction_type
{
    return viua::compose_bits_into<instruction_type>(
        out.encode(), htole32(immediate), viua::compose_filler{ 8 }, opcode);
}
auto F::to_string() const -> std::string
{
    auto imm_str = std::to_string(immediate);
    using viua::arch::ops::OPCODE;
    if (static_cast<OPCODE>(opcode) == OPCODE::FLOAT) {
        auto tmp = float{};
        memcpy(&tmp, &immediate, sizeof(immediate));
        imm_str = std::to_string(tmp);
    }
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + imm_str);
}
}  // namespace viua::arch::ops
