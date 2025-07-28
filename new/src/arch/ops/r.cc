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
R::R(
    viua::arch::opcode_type const op,
    Register_access const o,
    Register_access const i,
    uint32_t const im)
    : opcode{ op }
    , out{ o }
    , in{ i }
    , immediate{ im }
{}
auto R::decode(
    instruction_type const raw) -> R
{
    auto const opcode = carve_opcode_out(raw);
    auto const dst = carve_bits_out<Register_access::underlying_type, 16>(raw);
    auto const src = carve_bits_out<Register_access::underlying_type, 24>(raw);
    auto const immediate = carve_bits_out<uint32_t, 32>(raw);

    return R{ opcode,
              Register_access::decode(dst),
              Register_access::decode(src),
              le32toh(immediate) };
}
auto R::encode() const -> instruction_type
{
    return viua::compose_bits_into<instruction_type>(
        opcode, out.encode(), in.encode(), htole32(immediate));
}
auto R::to_string() const -> std::string
{
    auto const as_unsigned = (opcode & viua::arch::ops::UNSIGNED);
    auto imm_str           = as_unsigned
                                 ? std::to_string(immediate)
                                 : std::to_string(static_cast<int32_t>(immediate));
    return (viua::arch::ops::to_string(opcode) + " " + out.to_string() + ", "
            + in.to_string() + ", " + imm_str);
}
}  // namespace viua::arch::ops
