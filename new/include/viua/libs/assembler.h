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

#ifndef VIUA_LIBS_ASSEMBLER_H
#define VIUA_LIBS_ASSEMBLER_H

#include <stdint.h>     // for uintX_t
#include <sys/types.h>  // for size_t, see system_data_types(7)

#include <utility>


namespace viua::libs::assembler {
auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint32_t, uint32_t>;
auto li_cost(uint64_t const value) -> size_t;
}  // namespace viua::libs::assembler

#endif
