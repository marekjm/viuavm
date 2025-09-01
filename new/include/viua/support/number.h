/*
 *  Copyright (C) 2023 Marek Marecki
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

#ifndef VIUA_SUPPORT_NUMBER_H
#define VIUA_SUPPORT_NUMBER_H

#include <charconv>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>


namespace viua::support {
template<typename To, typename From>
auto sign_extend(
    From const v) -> To
{
    if constexpr (sizeof(To) <= sizeof(From)) {
        return static_cast<To>(v);
    } else {
        constexpr auto sign_extend_shift = (sizeof(To) - sizeof(From)) * 8;
        return (static_cast<To>(v) << sign_extend_shift) >> sign_extend_shift;
    }
}

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
auto ston(
    std::string n) -> T
{
    if constexpr (std::is_floating_point_v<T>) {
        if constexpr (sizeof(T) == sizeof(float)) {
            return std::stof(n);
        } else {
            return std::stod(n);
        }
    } else {
        auto view = std::string_view{ n };

        auto const is_negative = view.starts_with("-");
        if (is_negative and not std::is_signed_v<T>) {
            throw std::logic_error{ "ston" };
        }
        if (is_negative) {
            view.remove_prefix(1);
        }

        constexpr auto default_base = 10;
        auto base                   = default_base;
        if (view.starts_with("0x")) {
            base = 16;
        } else if (view.starts_with("0o")) {
            base = 8;
        } else if (view.starts_with("0b")) {
            base = 2;
        }

        /*
         * std::from_chars does not accept prefixes (0x, 0b, 0o) if the base is
         * set explicitly. Since we do set the base explicitly, we have to erase
         * the prefix.
         */
        if (base != default_base) {
            n.erase(static_cast<size_t>(is_negative), 2);
        }

        /*
         * Syntax supports ' as digit separator, but std::from_chars does not.
         */
        std::erase(n, '\'');

        auto value        = T{};
        auto const first  = n.c_str();
        auto const last   = first + n.size();
        auto const result = std::from_chars(first, last, value, base);

        if (result.ec == std::errc::invalid_argument) {
            throw std::invalid_argument{ "ston" };
        }
        if (result.ec == std::errc::result_out_of_range) {
            throw std::out_of_range{ "ston" };
        }

        return value;
    }
}

template<typename T>
auto bool_of_float(
    T const v) -> bool
{
    static_assert(std::is_floating_point<T>::value, "not a float");
    static_assert(
        ((sizeof(T) == sizeof(uint32_t)) or (sizeof(T) == sizeof(uint64_t))),
        "bad-sized float");
    using int_type = std::
        conditional<(sizeof(T) == sizeof(uint32_t)), uint32_t, uint64_t>::type;

    /*
     * For casting to bool the roundabout way of memcpy(3) into a
     * suitably-sized integer and casting that integer has to be used.
     * Otherwise the -Wfloat-equal warning gets triggered on GCC and I
     * cannot get a clean, warning-free compilation.
     */
    auto tmp = int_type{};
    memcpy(&tmp, &v, sizeof(T));
    return static_cast<bool>(tmp);
}
}  // namespace viua::support

#endif
