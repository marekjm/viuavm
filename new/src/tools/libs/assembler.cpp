/*
 *  Copyright (C) 2022 Marek Marecki
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

#include <viua/libs/assembler.h>


namespace viua::libs::assembler {
namespace {
constexpr auto LOW_32  = uint64_t{0x00000000ffffffff};
constexpr auto HIGH_32 = uint64_t{0xffffffff00000000};
}  // namespace

auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint32_t, uint32_t>
{
    auto const hi_part = static_cast<uint32_t>((value & HIGH_32) >> 32);
    auto const lo_part = static_cast<uint32_t>((value & LOW_32) >> 0);
    return {hi_part, lo_part};
}
auto li_cost(uint64_t const value) -> size_t
{
    auto const high_part = static_cast<bool>(value & HIGH_32);
    auto const low_part  = static_cast<bool>(value & LOW_32);
    return static_cast<size_t>(high_part) + static_cast<size_t>(low_part);
}
}  // namespace viua::libs::assembler
