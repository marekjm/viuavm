/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#pragma once

#include <stdint.h>

namespace viua {
    namespace internals {
        namespace types {
            typedef uint8_t byte;

            typedef uint64_t bytecode_size;
            typedef uint32_t register_index;

            typedef uint32_t schedulers_count;
            typedef uint64_t processes_count;

            typedef uint16_t process_time_slice_type;

            typedef int32_t plain_int;
            typedef double plain_float;

            typedef uint32_t timeout;

            typedef uint8_t registerset_type_marker;
        }

        enum class RegisterSets: types::registerset_type_marker {
            CURRENT = 0,
            GLOBAL,
            LOCAL,
            STATIC,
            TEMPORARY,
        };
    }
}

#endif
