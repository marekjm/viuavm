/*
 *  Copyright (C) 2019, 2020 Marek Marecki
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

#ifndef VIUA_BYTECODE_CODEC_H
#define VIUA_BYTECODE_CODEC_H

#include <cstdint>
#include <tuple>

namespace viua { namespace bytecode { namespace codec {
enum class Operand_type {
    /*
     * Void operands are used when the instruction should receive "nothing"
     * in an operand. For example, when a return value of a call should be
     * discarded a void operand is used.
     */
    Void,

    /*
     * The most common type of an operand. Used when the instruction should
     * fetch its input from a register, or to specify the register inside
     * which the instruction should place its output.
     */
    Register,

    /*
     * A simple true-false value.
     */
    Bool,

    /*
     * Basic numeric types.
     */
    Integer,
    Float,

    /*
     * Bits are used by VM-implemented arithmetic instructions or for
     * bitwise operations like shifts and rotates.
     */
    Bits,

    /*
     * Strings represent all kinds of operands that may be encoded as a
     * variable-length byte array. This means that raw byte strings, text,
     * atoms, and symbols (module, function, and block names) are all considered
     * to be strings. This saves space
     */
    String,

    /*
     * A literal timeout. Used in operations that may block (e.g. receiving
     * a message, or waiting for I/O to change status).
     */
    Timeout,
};

enum class Register_set : uint8_t {
    Global = 0,
    Local,
    Static,
    Arguments,
    Parameters,
    Closure_local,
};

using register_index_type = uint32_t;
using timeout_type        = uint32_t;

enum class Access_specifier {
    Direct              = 0,
    Register_indirect   = 1,
    Pointer_dereference = 2,
};

using Register_access =
    std::tuple<Register_set, register_index_type, Access_specifier>;
}}}  // namespace viua::bytecode::codec

#endif
