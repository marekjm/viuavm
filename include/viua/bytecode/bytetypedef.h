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

/** This header contains only one typedef - for byte.
 *
 *  This is because this typedef is required by various files across Viua
 *  source code, and often they do not need the opcodes.h header.
 *  This header prevents redefinition of `byte` in every file and
 *  helps keeping the definition consistent.
 *
 */

typedef uint8_t byte;

namespace viua {
    namespace internals {
        namespace types {
            // FIXME should be uint8_t
            typedef unsigned register_index;

            typedef int plain_int;
            typedef double plain_float;

            typedef unsigned timeout;
        }
    }
}

#endif
