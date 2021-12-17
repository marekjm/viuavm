/*
 *  Copyright (C) 2021 Marek Marecki
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

#ifndef VIUA_ARCH_OPS_H
#define VIUA_ARCH_OPS_H

#include <stdint.h>

#include <string>

#include <viua/arch/arch.h>


namespace viua::arch::ops {
constexpr auto FORMAT_N = opcode_type{0x0000};
constexpr auto FORMAT_T = opcode_type{0x1000};
constexpr auto FORMAT_D = opcode_type{0x2000};
constexpr auto FORMAT_S = opcode_type{0x3000};
constexpr auto FORMAT_F = opcode_type{0x4000};
constexpr auto FORMAT_E = opcode_type{0x5000};
constexpr auto FORMAT_R = opcode_type{0x6000};

/*
 * Create an enum to make use of switch statement's exhaustiveness checks.
 * Without an enum the compiler will not perform them. The intended usage
 * is:
 *
 *  - FORMAT::X whenever a strong check is needed (ie, in switch statement)
 *  - FORMAT_X whenever an integer is needed
 *
 * static_cast between them as appropriate.
 */
enum class FORMAT : opcode_type {
    N = FORMAT_N,
    T = FORMAT_T,
    D = FORMAT_D,
    S = FORMAT_S,
    F = FORMAT_F,
    E = FORMAT_E,
    R = FORMAT_R,
};
auto to_string(FORMAT const) -> std::string;

/*
 * Three-way (triple) register access.
 */
struct T {
    viua::arch::opcode_type opcode;
    Register_access const out;
    Register_access const lhs;
    Register_access const rhs;

    T(viua::arch::opcode_type const,
      Register_access const,
      Register_access const,
      Register_access const);

    static auto decode(instruction_type const) -> T;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * Two-way (double) register access.
 */
struct D {
    viua::arch::opcode_type opcode;
    Register_access const out;
    Register_access const in;

    D(viua::arch::opcode_type const,
      Register_access const,
      Register_access const);

    static auto decode(instruction_type const) -> D;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * One-way (single) register access.
 */
struct S {
    viua::arch::opcode_type opcode;
    Register_access const out;

    S(viua::arch::opcode_type const, Register_access const);

    static auto decode(instruction_type const) -> S;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * One-way register access with 32-bit wide immediate value.
 * "F" because it is used for eg, floats.
 */
struct F {
    viua::arch::opcode_type opcode;
    Register_access const out;
    uint32_t const immediate;

    F(viua::arch::opcode_type const op,
      Register_access const o,
      uint32_t const i);

    template<typename T>
    static auto make(viua::arch::opcode_type const op,
                     Register_access const o,
                     T const v) -> F
    {
        static_assert(sizeof(T) == sizeof(uint32_t));
        auto imm = uint32_t{};
        memcpy(&imm, &v, sizeof(imm));
        return F{op, o, imm};
    }

    static auto decode(instruction_type const) -> F;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * One-way register access with 36-bit wide immediate value.
 * "E" because it is "extended" immediate, 4 bits longer than the F format.
 */
struct E {
    viua::arch::opcode_type opcode;
    Register_access const out;
    uint64_t const immediate;

    E(viua::arch::opcode_type const op,
      Register_access const o,
      uint64_t const i);

    static auto decode(instruction_type const) -> E;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * Two-way register access with 24-bit wide immediate value.
 * "R" because it is "reduced" immediate, 8 bits shorter than the F format.
 */
struct R {
    viua::arch::opcode_type opcode;
    Register_access const out;
    Register_access const in;
    uint32_t const immediate;

    R(viua::arch::opcode_type const,
      Register_access const,
      Register_access const,
      uint32_t const);

    static auto decode(instruction_type const) -> R;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * No operands.
 */
struct N {
    viua::arch::opcode_type opcode;

    N(viua::arch::opcode_type const);

    static auto decode(instruction_type const) -> N;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

constexpr auto GREEDY      = opcode_type{0x8000};
constexpr auto UNSIGNED    = opcode_type{0x0800};
constexpr auto OPCODE_MASK = opcode_type{0x7fff};
constexpr auto FORMAT_MASK = opcode_type{0x7000};

enum class OPCODE : opcode_type {
    NOOP   = (FORMAT_N | 0x0000),
    HALT   = (FORMAT_N | 0x0001),
    EBREAK = (FORMAT_N | 0x0002),

    ADD = (FORMAT_T | 0x0001),
    SUB = (FORMAT_T | 0x0002),
    MUL = (FORMAT_T | 0x0003),
    DIV = (FORMAT_T | 0x0004),
    MOD = (FORMAT_T | 0x0005),

    BITSHL  = (FORMAT_T | 0x0006),
    BITSHR  = (FORMAT_T | 0x0007),
    BITASHR = (FORMAT_T | 0x0008),
    BITROL  = (FORMAT_T | 0x0009),
    BITROR  = (FORMAT_T | 0x000a),
    BITAND  = (FORMAT_T | 0x000b),
    BITOR   = (FORMAT_T | 0x000c),
    BITXOR  = (FORMAT_T | 0x000d),

    EQ  = (FORMAT_T | 0x000e),
    LT  = (FORMAT_T | 0x000f),
    GT  = (FORMAT_T | 0x0010),
    CMP = (FORMAT_T | 0x0011),
    AND = (FORMAT_T | 0x0012),
    OR  = (FORMAT_T | 0x0013),

    BUFFER_AT  = (FORMAT_T | 0x0014),
    BUFFER_POP = (FORMAT_T | 0x0015),

    STRUCT_AT     = (FORMAT_T | 0x0016),
    STRUCT_INSERT = (FORMAT_T | 0x0017),
    STRUCT_REMOVE = (FORMAT_T | 0x0018),

    CALL   = (FORMAT_D | 0x0001),
    BITNOT = (FORMAT_D | 0x0002),
    NOT    = (FORMAT_D | 0x0003),

    COPY = (FORMAT_D | 0x0004),
    MOVE = (FORMAT_D | 0x0005),
    SWAP = (FORMAT_D | 0x0006),

    BUFFER_PUSH = (FORMAT_D | 0x0007),
    BUFFER_SIZE = (FORMAT_D | 0x0008),

    REF = (FORMAT_D | 0x0009),

    IF = (FORMAT_D | 0x000a),

    FRAME  = (FORMAT_S | 0x0001),
    RETURN = (FORMAT_S | 0x0002),
    ATOM   = (FORMAT_S | 0x0003),
    STRING = (FORMAT_S | 0x0004),
    FLOAT  = (FORMAT_S | 0x0005),
    DOUBLE = (FORMAT_S | 0x0006),
    STRUCT = (FORMAT_S | 0x0007),
    BUFFER = (FORMAT_S | 0x0008),

    JUMP = (FORMAT_S | 0x0009),

    LUI  = (FORMAT_E | 0x0001),
    LUIU = (FORMAT_E | 0x0001 | UNSIGNED),

    ADDI  = (FORMAT_R | 0x0001),
    ADDIU = (FORMAT_R | 0x0001 | UNSIGNED),
    SUBI  = (FORMAT_R | 0x0002),
    SUBIU = (FORMAT_R | 0x0002 | UNSIGNED),
    MULI  = (FORMAT_R | 0x0003),
    MULIU = (FORMAT_R | 0x0003 | UNSIGNED),
    DIVI  = (FORMAT_R | 0x0004),
    DIVIU = (FORMAT_R | 0x0004 | UNSIGNED),
};
auto to_string(opcode_type const) -> std::string;
auto parse_opcode(std::string_view) -> opcode_type;

/*
 * These are helper enums to provide exhaustiveness checks for switch
 * statements on opcodes of one format.
 */
enum class OPCODE_T : opcode_type {
    ADD           = static_cast<opcode_type>(OPCODE::ADD),
    SUB           = static_cast<opcode_type>(OPCODE::SUB),
    MUL           = static_cast<opcode_type>(OPCODE::MUL),
    DIV           = static_cast<opcode_type>(OPCODE::DIV),
    MOD           = static_cast<opcode_type>(OPCODE::MOD),
    BITSHL        = static_cast<opcode_type>(OPCODE::BITSHL),
    BITSHR        = static_cast<opcode_type>(OPCODE::BITSHR),
    BITASHR       = static_cast<opcode_type>(OPCODE::BITASHR),
    BITROL        = static_cast<opcode_type>(OPCODE::BITROL),
    BITROR        = static_cast<opcode_type>(OPCODE::BITROR),
    BITAND        = static_cast<opcode_type>(OPCODE::BITAND),
    BITOR         = static_cast<opcode_type>(OPCODE::BITOR),
    BITXOR        = static_cast<opcode_type>(OPCODE::BITXOR),
    EQ            = static_cast<opcode_type>(OPCODE::EQ),
    LT            = static_cast<opcode_type>(OPCODE::LT),
    GT            = static_cast<opcode_type>(OPCODE::GT),
    CMP           = static_cast<opcode_type>(OPCODE::CMP),
    AND           = static_cast<opcode_type>(OPCODE::AND),
    OR            = static_cast<opcode_type>(OPCODE::OR),
    BUFFER_AT     = static_cast<opcode_type>(OPCODE::BUFFER_AT),
    BUFFER_POP    = static_cast<opcode_type>(OPCODE::BUFFER_POP),
    STRUCT_AT     = static_cast<opcode_type>(OPCODE::STRUCT_AT),
    STRUCT_INSERT = static_cast<opcode_type>(OPCODE::STRUCT_INSERT),
    STRUCT_REMOVE = static_cast<opcode_type>(OPCODE::STRUCT_REMOVE),
};
enum class OPCODE_D : opcode_type {
    CALL        = static_cast<opcode_type>(OPCODE::CALL),
    BITNOT      = static_cast<opcode_type>(OPCODE::BITNOT),
    NOT         = static_cast<opcode_type>(OPCODE::NOT),
    COPY        = static_cast<opcode_type>(OPCODE::COPY),
    MOVE        = static_cast<opcode_type>(OPCODE::MOVE),
    SWAP        = static_cast<opcode_type>(OPCODE::SWAP),
    BUFFER_PUSH = static_cast<opcode_type>(OPCODE::BUFFER_PUSH),
    BUFFER_SIZE = static_cast<opcode_type>(OPCODE::BUFFER_SIZE),
    REF         = static_cast<opcode_type>(OPCODE::REF),
    IF          = static_cast<opcode_type>(OPCODE::IF),
};
enum class OPCODE_S : opcode_type {
    FRAME  = static_cast<opcode_type>(OPCODE::FRAME),
    RETURN = static_cast<opcode_type>(OPCODE::RETURN),
    ATOM   = static_cast<opcode_type>(OPCODE::ATOM),
    STRING = static_cast<opcode_type>(OPCODE::STRING),
    FLOAT  = static_cast<opcode_type>(OPCODE::FLOAT),
    DOUBLE = static_cast<opcode_type>(OPCODE::DOUBLE),
    STRUCT = static_cast<opcode_type>(OPCODE::STRUCT),
    BUFFER = static_cast<opcode_type>(OPCODE::BUFFER),
    JUMP = static_cast<opcode_type>(OPCODE::JUMP),
};
enum class OPCODE_E : opcode_type {
    LUI  = static_cast<opcode_type>(OPCODE::LUI),
    LUIU = static_cast<opcode_type>(OPCODE::LUIU),
};
enum class OPCODE_R : opcode_type {
    ADDI  = static_cast<opcode_type>(OPCODE::ADDI),
    ADDIU = static_cast<opcode_type>(OPCODE::ADDIU),
    SUBI  = static_cast<opcode_type>(OPCODE::SUBI),
    SUBIU = static_cast<opcode_type>(OPCODE::SUBIU),
    MULI  = static_cast<opcode_type>(OPCODE::MULI),
    MULIU = static_cast<opcode_type>(OPCODE::MULIU),
    DIVI  = static_cast<opcode_type>(OPCODE::DIVI),
    DIVIU = static_cast<opcode_type>(OPCODE::DIVIU),
};
enum class OPCODE_N : opcode_type {
    NOOP   = static_cast<opcode_type>(OPCODE::NOOP),
    HALT   = static_cast<opcode_type>(OPCODE::HALT),
    EBREAK = static_cast<opcode_type>(OPCODE::EBREAK),
};
}  // namespace viua::arch::ops

#endif
