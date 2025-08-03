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
#include <type_traits>
#include <utility>
#include <vector>


namespace viua {
namespace arithmetic {
struct zero_type {};

struct arithmetic_type {
    using bit_type   = bool;
    using value_type = std::vector<bit_type>;
    using size_type  = value_type::size_type;

    value_type n{};

    arithmetic_type()
    {}
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    explicit arithmetic_type(
        T const v)
    {
        std::bitset<sizeof(T) * 8> bs{ static_cast<std::make_unsigned_t<T>>(
            v) };
        n.resize(bs.size());
        for (auto i = size_t{ 0 }; i < bs.size(); ++i) {
            n.at(i) = bs.test(i);
        }
    }

    explicit operator bool() const;

    inline auto operator[](size_type const i) const -> bit_type
    {
        return n[i];
    }

    auto size() const -> size_type;
    auto test(size_type const) const -> bool;
    auto at(size_type const) const -> bit_type;

    auto push_back(bit_type const) -> void;
};
using size_type = arithmetic_type::size_type;

/*
 * Sign-extend by default. Use the optional expander value to override.
 */
auto extend(arithmetic_type,
            size_type const,
            std::optional<bool> const = std::nullopt) -> arithmetic_type;


struct signed_type {
    using bit_type = arithmetic_type::bit_type;
    using value_type = arithmetic_type;
    using size_type  = value_type::size_type;

    value_type n;

    template<typename T, typename = std::enable_if_t<std::is_signed_v<T>>>
    explicit signed_type(
        T const v)
        : n{ v }
    {}
    inline explicit signed_type(
        arithmetic_type v)
        : n{ std::move(v) }
    {}

    template<typename T, typename = std::enable_if_t<std::is_signed_v<T>>>
    explicit operator T() const
    {
        constexpr auto target_width = (sizeof(T) * 8);
        if constexpr (false) {
            if (target_width < n.size()) {
                throw std::runtime_error{
                    "narrowing direct static_cast from signed_type"
                };
            }
        }

        auto const fixed = extend(n, target_width);
        std::bitset<target_width> bs{};
        for (auto i = size_type{ 0 }; i < fixed.size(); ++i) {
            bs.set(i, fixed.at(i));
        }
        return static_cast<T>(bs.to_ullong());
    }

    explicit operator bool() const;

    auto operator~() const -> signed_type;
    inline auto operator[](size_type const i) const -> bit_type
    {
        return n[i];
    }

    auto size() const -> size_type;
};
auto operator<(signed_type const, zero_type const) -> bool;

auto operator<(signed_type const, signed_type const) -> bool;
auto operator==(signed_type const, signed_type const) -> bool;


struct unsigned_type {
    using value_type = arithmetic_type;
    using size_type  = value_type::size_type;

    value_type n;

    template<typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
    unsigned_type(
        T const v)
        : n{ v }
    {}
};


inline constexpr auto DEFAULT_SEPARATOR = std::pair{ 4, '\'' };
auto to_string(arithmetic_type const,
               bool const with_prefix                          = false,
               std::optional<std::pair<size_type, char>> const = std::nullopt)
    -> std::string;
inline auto to_string(
    signed_type const v,
    bool const with_prefix                              = false,
    std::optional<std::pair<size_type, char>> const sep = std::nullopt)
    -> std::string
{
    return to_string(v.n, with_prefix, sep);
}
inline auto to_string(
    unsigned_type const v,
    bool const with_prefix                              = false,
    std::optional<std::pair<size_type, char>> const sep = std::nullopt)
    -> std::string
{
    return to_string(v.n, with_prefix, sep);
}

auto take_twos_complement(arithmetic_type const) -> arithmetic_type;


namespace bits {
auto shr(arithmetic_type const, size_type const) -> arithmetic_type;
auto shl(arithmetic_type const, size_type const) -> arithmetic_type;

auto inc(arithmetic_type) -> arithmetic_type;
auto dec(arithmetic_type) -> arithmetic_type;

auto add(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto sub(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto mul(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto div(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
}  // namespace bits

namespace fixed {
auto operator+(signed_type const, signed_type const) -> signed_type;
auto operator-(signed_type const, signed_type const) -> signed_type;
}  // namespace fixed

namespace saturating {
auto operator+(signed_type const, signed_type const) -> signed_type;
auto operator-(signed_type const, signed_type const) -> signed_type;
}  // namespace saturating

#if 0

auto clip(arithmetic_type const, size_type const) -> arithmetic_type;

auto invert(arithmetic_type) -> arithmetic_type;
auto zero(arithmetic_type const) -> arithmetic_type;

auto is_zero(arithmetic_type const) -> bool;
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

namespace saturating {
auto inc(arithmetic_type) -> arithmetic_type;
auto dec(arithmetic_type) -> arithmetic_type;

auto add(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto sub(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto mul(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
auto div(arithmetic_type const, arithmetic_type const) -> arithmetic_type;
}
#endif
}  // namespace arithmetic
}  // namespace viua

#endif
