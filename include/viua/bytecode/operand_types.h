/*
 *  Copyright (C) 2015, 2016, 2020 Marek Marecki
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

#ifndef VIUA_OPERAND_TYPES_H
#define VIUA_OPERAND_TYPES_H

#include <stdint.h>

enum OperandType : uint8_t {
    OT_INVALID            = 0,
    OT_REGISTER_INDEX     = 1,  // register index
    OT_REGISTER_REFERENCE = 2,  // register reference (indirect register index)
    OT_REGISTER_INDEX_ANNOTATED = 3,  // register index with an annotation from
                                      // which register set it should be taken

    OT_POINTER = 4,  // index of register containing a pointer that
                     // should be dereferenced (should be decoded to
                     // dereferenced value)

    OT_ATOM   = 5,  // null-terminated ASCII string
    OT_TEXT   = 6,  // UTF-8 encoded Unicode string, null-terminated
    OT_STRING = 7,  // size-prefixed byte string
    OT_BITS   = 8,  // size-prefixed bit string

    OT_INT = 9,  // unlimited size signed integer

    OT_FLOAT = 10,  // unlimited precision floating point number

    OT_VOID = 11,  // encodes the abstract concept of "nothing"

    OT_BOOL  = 128,
    OT_TRUE  = 12 | OT_BOOL,  // encodes literal true
    OT_FALSE = 13 | OT_BOOL,  // encodes literal false
};

namespace viua { namespace internals {
enum class Access_specifier {
    DIRECT              = 0,
    REGISTER_INDIRECT   = 1,
    POINTER_DEREFERENCE = 2,
};

using ValueTypesType = uint32_t;
enum class Value_types : ValueTypesType {
    /*
     * This is the type that is used when it is not known what type a value
     * actually has. When the type can be inferred UNDEFINED may be swapped for
     * the actual type.
     */
    UNDEFINED = 0,

    /*
     * This is the type of void value.
     * It is not actually used (as it is not possible to create a void value),
     * but is defined here for the sake of completeness.
     */
    VOID = 1 << 0,

    /*
     * Numeric types.
     *
     * Some instructions require an INTEGER (e.g. "iinc" and "idec", bit
     * shifts), and some accept both INTEGER and FLOAT values (e.g. arithmetic
     * ops). The NUMBER alias is used for these "relaxed" ops that accept
     * either.
     */
    INTEGER = 1 << 2,
    FLOAT   = 1 << 3,
    NUMBER  = (INTEGER | FLOAT),

    /*
     * Produced by comparison ops (e.g. "eq", "lt", "texteq", "atomeq").
     */
    BOOLEAN = 1 << 4,

    /*
     * Type of UTF-8 encoded Unicode text.
     */
    TEXT = 1 << 5,

    /*
     * Type of a basic string.
     * FIXME DEPRECATED
     */
    STRING = 1 << 6,

    VECTOR = 1 << 7,

    BITS = 1 << 8,

    FUNCTION  = 1 << 9,
    CLOSURE   = 1 << 10,
    INVOCABLE = (FUNCTION | CLOSURE),

    ATOM = 1 << 11,

    PID = 1 << 12,

    STRUCT = 1 << 13,

    POINTER = 1 << 15,

    IO_REQUEST   = 1 << 16,
    IO_PORT      = 1 << 17,
    IO_PORT_LIKE = (IO_PORT | INTEGER),

    EXCEPTION = 1 << 18,
};
}}  // namespace viua::internals

#endif
