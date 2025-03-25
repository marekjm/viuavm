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
F::F(viua::arch::opcode_type const op,
     Register_access const o,
     uint32_t const i)
        : opcode{op}, out{o}, immediate{i}
{}
auto F::decode(instruction_type const raw) -> F
{
    auto opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto out   = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto value = le32toh(static_cast<uint32_t>(raw >> 32));
    return F{opcode, out, value};
}
auto F::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto value           = uint64_t{htole32(immediate)};
    return base | (output_register << 16) | (value << 32);
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
