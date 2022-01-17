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

#ifndef VIUA_ARCH_ARCH_H
#define VIUA_ARCH_ARCH_H

#include <stddef.h>
#include <stdint.h>

#include <sstream>
#include <string>


namespace viua::arch {
/*
 * Operation codes are encoded on 16 bits.
 * The main part of the opcode is encoded on the lowest 15 bits. The
 * highest bit is a "greedy bit" - a suggestion to the scheduler that it
 * should execute the next instruction immediately, instead of preempting a
 * process.
 */
using opcode_type = uint16_t;

/*
 * Instructions are encoded on 64 bits.
 * Full instruction is composed of the opcode and the operands. Having all
 * instructions be the same width makes it easier to count jump widths and
 * allocate memory for instructions, but imposes some restrictions. For
 * example, its impossible to load a 64 bit integer using a single
 * instruction.
 */
using instruction_type = uint64_t;

constexpr auto REGISTER_WIDTH = size_t{64};
using register_type = uint64_t;

enum class REGISTER_SET {
    /*
     * Void register used as an input register means that the instruction
     * should use a default value for an operand. For example:
     *
     *      addi $1, void, 42
     *
     * The instruction above makes the local register 1 contain signed
     * integer value 42, because it adds 42 to 0 (defaulted from void).
     *
     * Void register used as an output register means that the value
     * that would be produced and put inside it should be dropped instead.
     */
    VOID = 0,

    /*
     * Local registers are used to store local variables of a function.
     * Every call allocates a fresh local register set, and call frames do
     * not share registers (every call frame has its own local register set
     * allocated).
     */
    LOCAL,

    /*
     * Parameter registers are used by callees to read the values assigned
     * to their FORMAL PARAMETERS by their caller. They are read-only
     * registers (the values they contain can be moved out of them).
     *
     * Argument registers are used by callers to "make arguments", to assign
     * values for the ACTUAL PARAMETERS before a function call. These actual
     * parameters are available to callees through parameter registers.
     *
     * On the caller side:
     *
     *      move $1.a, *produced value*
     *      call void, some_function
     *
     * On the callee side:
     *
     *      move $1, $1.p
     *
     * The argument registers are moved into parameter registers on a CALL
     * INSTRUCTION of any kind.
     */
    ARGUMENT,
    PARAMETER,
};
using RS = REGISTER_SET;

using register_index_type         = uint8_t;
constexpr auto MAX_REGISTER_INDEX = register_index_type{255};

struct Register_access {
    using set_type = viua::arch::REGISTER_SET;

    set_type set;
    bool direct;
    uint8_t index;

    Register_access();
    Register_access(set_type const, bool const, uint8_t const);

    static auto decode(uint16_t const) -> Register_access;
    auto encode() const -> uint16_t;

    auto operator==(Register_access const& other) const -> bool
    {
        return (set == other.set) and (direct == other.direct)
               and (index == other.index);
    }

    inline auto is_legal() const -> bool
    {
        if (is_void() and not direct and index != 0) {
            return false;
        }
        return true;
    }

    inline auto is_void() const -> bool
    {
        return (set == set_type::VOID);
    }

    static auto make_local(uint8_t const, bool const = true) -> Register_access;
    static auto make_argument(uint8_t const, bool const = true)
        -> Register_access;
    static auto make_parameter(uint8_t const, bool const = true)
        -> Register_access;
    static auto make_void() -> Register_access;

    auto to_string() const -> std::string;
};
}  // namespace viua::arch

#endif
