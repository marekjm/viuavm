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

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>


namespace viua::support {
namespace {
template<typename T>
auto ston_int_impl(std::string const& n, int const base)
    -> std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>
{
    if constexpr (std::is_signed_v<T>) {
        return std::stoll(n, nullptr, base);
    } else {
        return std::stoull(n, nullptr, base);
    }
}
}  // namespace

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
auto ston(std::string n) -> T
{
    if constexpr (std::is_floating_point_v<T>) {
        if constexpr (sizeof(T) == sizeof(float)) {
            return std::stof(n);
        } else {
            return std::stod(n);
        }
    } else {
        auto base = 10;
        {
            auto view = std::string_view{n};
            auto const is_negative =
                std::is_signed_v<T> and view.starts_with("-");
            if (is_negative) {
                view.remove_prefix(1);
            }
            if (view.starts_with("0x")) {
                base = 16;
            } else if (view.starts_with("0o")) {
                base = 8;

                /*
                 * Octal literals in Viua assembly begin with "0o", but C++
                 * converters take octals beginning with "0". We need to drop
                 * the "o".
                 */
                n.erase(1 + static_cast<size_t>(is_negative), 1);
            } else if (view.starts_with("0b")) {
                base = 2;
            }
        }

        auto const full   = ston_int_impl<T>(n, base);
        auto const wanted = static_cast<T>(full);

        if (wanted != full) {
            throw std::out_of_range{"ston"};
        }
        return wanted;
    }
}
}  // namespace viua::support

#endif
