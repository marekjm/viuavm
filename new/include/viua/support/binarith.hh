/*
 *  Copyright (C) 2017-2019, 2025 Marek Marecki
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

#ifndef VIUA_SUPPORT_BINARITH_HH
#define VIUA_SUPPORT_BINARITH_HH

#include <bitset>
#include <optional>
#include <string>
#include <vector>
#include <utility>


namespace viua {
namespace arithmetic {
using arithmetic_type = std::vector<bool>;
using size_type = arithmetic_type::size_type;

template<typename T>
auto from_integer(T const v) -> arithmetic_type
{
    std::bitset<sizeof(T) * 8> bs { static_cast<uint8_t>(v) };
    auto a = arithmetic_type{};
    a.resize(bs.size());
    for (auto i = size_t{0}; i < bs.size(); ++i) {
        a.at(i) = bs.test(i);
    }
    return a;
}

auto to_string(
      arithmetic_type const
    , bool const with_prefix = false
    , std::optional<std::pair<size_type, char>> const = std::nullopt)
    -> std::string;

auto to_unsigned(arithmetic_type const) -> uint64_t;
auto to_signed(arithmetic_type const) -> int64_t;

auto to_bool(arithmetic_type const) -> bool;

/*
 * Sign-extend by default. Use the optional expander value to override.
 */
auto extend(arithmetic_type, size_type const, std::optional<bool> const = std::nullopt) -> arithmetic_type;

auto take_twos_complement(arithmetic_type const) -> arithmetic_type;

auto clip(arithmetic_type const, size_type const) -> arithmetic_type;

auto invert(arithmetic_type) -> arithmetic_type;
auto zero(arithmetic_type const) -> arithmetic_type;

auto is_negative(arithmetic_type const) -> bool;
auto highest_bit_set(arithmetic_type const) -> std::optional<size_type>;

namespace bits {
auto shr(arithmetic_type const, size_type const) -> arithmetic_type;
auto shl(arithmetic_type const, size_type const) -> arithmetic_type;

auto inc(arithmetic_type) -> arithmetic_type;
auto dec(arithmetic_type) -> arithmetic_type;

auto add(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto sub(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto mul(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto div(arithmetic_type const, arithmetic_type const) -> arithmetic_type;

auto eq(arithmetic_type const, arithmetic_type const) -> bool;
auto lt(arithmetic_type const, arithmetic_type const) -> bool;
auto lte(arithmetic_type const, arithmetic_type const) -> bool;
}

namespace fixed {
using with_carry_type = std::pair<bool, arithmetic_type>;

auto inc(arithmetic_type) -> with_carry_type;
auto dec(arithmetic_type) -> with_carry_type;

auto add(arithmetic_type const, arithmetic_type const) -> with_carry_type;
auto sub(arithmetic_type const, arithmetic_type const) -> with_carry_type;
auto mul(arithmetic_type const, arithmetic_type const) -> with_carry_type;
auto div(arithmetic_type const, arithmetic_type const) -> with_carry_type;
}
}
}

#endif
