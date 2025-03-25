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
N::N(viua::arch::opcode_type const op) : opcode{op}
{}
auto N::decode(instruction_type const raw) -> N
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);

    return N{opcode};
}
auto N::encode() const -> instruction_type
{
    return uint64_t{opcode};
}
}  // namespace viua::arch::ops
