/*
 *  Copyright (C) 2015-2018, 2020 Marek Marecki
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

#ifndef VIUA_BYTECODE_BYTETYPEDEF_H
#define VIUA_BYTECODE_BYTETYPEDEF_H

#include <stdint.h>

namespace viua { namespace internals {
namespace types {
using Op_address_type = uint8_t const*;

typedef uint16_t process_time_slice_type;
}  // namespace types
}}  // namespace viua::internals

#endif
