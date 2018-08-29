/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

namespace viua { namespace internals {
namespace types {
typedef uint8_t byte;
using Op_address_type = byte const*;

typedef uint64_t bytecode_size;
typedef uint32_t register_index;

typedef uint32_t schedulers_count;
typedef uint64_t processes_count;

typedef uint16_t process_time_slice_type;

typedef int32_t plain_int;
typedef double plain_float;

typedef uint32_t timeout;

typedef uint8_t registerset_type_marker;

typedef uint64_t bits_size;
}  // namespace types

enum class Register_sets : types::registerset_type_marker {
    GLOBAL = 0,
    LOCAL,
    STATIC,

    /*
     * As described in "Programming Language Pragmatics" by Michael L. Scott:
     *
     *      Most subroutines are parametrized: the caller passes arguments that
     *      influence the subroutine's behaviour, or provide it with data on which
     *      to operate. Arguments are also called actual parameters. They are
     *      mapped to the subroutine's formal parameters at the time a call occurs.
     *
     * This can be found on page 427, in the introduction to Chapter 8 "Subroutines
     * and Control Abstraction". The ISBN for the book is 1-55860-442-1.
     */
    ARGUMENTS,  // actual parameters (arguments):  what is passed to a function call
    PARAMETERS, // formal parameters (parameters): what the function body sees

    /*
     * Used in capturing instructions.
     */
    CLOSURE_LOCAL,
};
}}  // namespace viua::internals

#endif
