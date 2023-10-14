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

#ifndef VIUA_SUPPORT_MEMORY_H
#define VIUA_SUPPORT_MEMORY_H

#include <stdint.h>
#include <string.h>


namespace viua::support {
template<typename T>
auto memload(void const* const src) -> T
{
    auto tmp = T{};
    memcpy(&tmp, src, sizeof(T));
    return tmp;
}
}  // namespace viua::support::string

#endif
