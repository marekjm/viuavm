/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#ifndef VIUA_SUPPORT_VARIANT_H
#define VIUA_SUPPORT_VARIANT_H

#include <stddef.h>

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>


namespace viua::support {
/*
 * The type always contains a constexpr member variable `value' which evaluates
 * to false. It is, however, dependent on a type which prevents the compiler
 * from immediately evaluating the member variable and instead defers the check
 * to the point of instantiation (this is mostly useful in templates).
 *
 * The type makes it possible to use std::visit() in a semi-sane way.
 */
template<typename T> struct always_false : std::false_type {
};

template<typename T> constexpr auto non_exhaustive_visitor() -> void
{
    static_assert(always_false<T>::value, "non-exhaustive visitor");
}
}  // namespace viua::support

#endif
