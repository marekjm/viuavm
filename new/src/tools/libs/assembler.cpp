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
auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint64_t, std::pair<std::pair<uint32_t, uint32_t>, uint32_t>>
{
    constexpr auto LOW_24  = uint64_t{0x0000000000ffffff};
    constexpr auto HIGH_36 = uint64_t{0xfffffffff0000000};

    auto const high_part = ((value & HIGH_36) >> 28);
    auto const low_part  = static_cast<uint32_t>(value & ~HIGH_36);

    /*
     * If the low part consists of only 24 bits we can use just two
     * instructions:
     *
     *  1/ lui to load high 36 bits
     *  2/ addi to add low 24 bits
     *
     * This reduces the overhead of loading 64-bit values.
     */
    if ((low_part & LOW_24) == low_part) {
        return {high_part, {{low_part, 0}, 0}};
    }

    auto const multiplier = 16;
    auto const remainder  = (low_part % multiplier);
    auto const base       = (low_part - remainder) / multiplier;

    return {high_part, {{base, multiplier}, remainder}};
}
auto li_cost(uint64_t const value) -> size_t
{
    /*
     * Remember to keep this function in sync with what expand_li() function
     * does. If this function ie, li_cost() returns a bad value, or expand_li()
     * starts emitting different instructions -- branch calculations will fail.
     */
    auto count = size_t{0};

    auto parts = to_loading_parts_unsigned(value);
    if (parts.first) {
        ++count;  // lui
    }

    auto const multiplier = parts.second.first.second;
    if (multiplier != 0) {
        ++count;  // g.addiu
        ++count;  // g.addiu
        ++count;  // g.mul
        ++count;  // g.addiu
        ++count;  // g.add
        ++count;  // g.add
        ++count;  // g.delete
        ++count;  // g.delete
    } else if (parts.second.first.first or (value == 0)) {
        ++count;  // addiu
    }

    return count;
}
}  // namespace viua::libs::assembler
