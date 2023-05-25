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
constexpr auto FORMAT_M = opcode_type{0x7000};

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
    M = FORMAT_M,
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
 * Two-way register access, with 16-bit memory offset, and 8-bit specifier.
 * "M" because it is used for loads and stored, which interact with "memory".
 */
struct M {
    viua::arch::opcode_type opcode;
    Register_access const out;
    Register_access const in;
    uint16_t const immediate;
    uint8_t const spec;

    M(viua::arch::opcode_type const,
      Register_access const,
      Register_access const,
      uint16_t const,
      uint8_t const);

    static auto decode(instruction_type const) -> M;
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
    ECALL  = (FORMAT_N | 0x0003),

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

    IO_SUBMIT   = (FORMAT_T | 0x0019),
    IO_WAIT     = (FORMAT_T | 0x001a),
    IO_SHUTDOWN = (FORMAT_T | 0x001b),
    IO_CTL      = (FORMAT_T | 0x001c),

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

    IO_PEEK = (FORMAT_D | 0x000b),

    ACTOR = (FORMAT_D | 0x000c),

    FRAME  = (FORMAT_S | 0x0001),
    RETURN = (FORMAT_S | 0x0002),
    ATOM   = (FORMAT_S | 0x0003),
    STRING = (FORMAT_S | 0x0004),
    FLOAT  = (FORMAT_S | 0x0005),
    DOUBLE = (FORMAT_S | 0x0006),
    STRUCT = (FORMAT_S | 0x0007),
    BUFFER = (FORMAT_S | 0x0008),

    SELF = (FORMAT_S | 0x0009),

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

    SM = (FORMAT_M | 0x0001),   /* Store Memory */
    LM = (FORMAT_M | 0x0002),   /* Load Memory */
    AA = (FORMAT_M | 0x0003),   /* Allocate Automatic */
    AD = (FORMAT_M | 0x0004),   /* Allocate Dynamic */
    PTR = (FORMAT_M | 0x0005),  /* PoinTeR */
};
auto to_string(opcode_type const) -> std::string;
auto parse_opcode(std::string_view) -> opcode_type;

/*
 * These are helper enums to provide exhaustiveness checks for switch
 * statements on opcodes of one format.
 */
#define Make_entry(OP) OP = static_cast<opcode_type>(OPCODE::OP)
enum class OPCODE_T : opcode_type {
    Make_entry(ADD),
    Make_entry(SUB),
    Make_entry(MUL),
    Make_entry(DIV),
    Make_entry(MOD),
    Make_entry(BITSHL),
    Make_entry(BITSHR),
    Make_entry(BITASHR),
    Make_entry(BITROL),
    Make_entry(BITROR),
    Make_entry(BITAND),
    Make_entry(BITOR),
    Make_entry(BITXOR),
    Make_entry(EQ),
    Make_entry(LT),
    Make_entry(GT),
    Make_entry(CMP),
    Make_entry(AND),
    Make_entry(OR),
    Make_entry(BUFFER_AT),
    Make_entry(BUFFER_POP),
    Make_entry(STRUCT_AT),
    Make_entry(STRUCT_INSERT),
    Make_entry(STRUCT_REMOVE),
    Make_entry(IO_SUBMIT),
    Make_entry(IO_WAIT),
    Make_entry(IO_SHUTDOWN),
    Make_entry(IO_CTL),
};
enum class OPCODE_D : opcode_type {
    Make_entry(CALL),
    Make_entry(BITNOT),
    Make_entry(NOT),
    Make_entry(COPY),
    Make_entry(MOVE),
    Make_entry(SWAP),
    Make_entry(BUFFER_PUSH),
    Make_entry(BUFFER_SIZE),
    Make_entry(REF),
    Make_entry(IF),
    Make_entry(IO_PEEK),
    Make_entry(ACTOR),
};
enum class OPCODE_S : opcode_type {
    Make_entry(FRAME),
    Make_entry(RETURN),
    Make_entry(ATOM),
    Make_entry(STRING),
    Make_entry(FLOAT),
    Make_entry(DOUBLE),
    Make_entry(STRUCT),
    Make_entry(BUFFER),
    Make_entry(SELF),
};
enum class OPCODE_E : opcode_type {
    Make_entry(LUI),
    Make_entry(LUIU),
};
enum class OPCODE_R : opcode_type {
    Make_entry(ADDI),
    Make_entry(ADDIU),
    Make_entry(SUBI),
    Make_entry(SUBIU),
    Make_entry(MULI),
    Make_entry(MULIU),
    Make_entry(DIVI),
    Make_entry(DIVIU),
};
enum class OPCODE_N : opcode_type {
    Make_entry(NOOP),
    Make_entry(HALT),
    Make_entry(EBREAK),
    Make_entry(ECALL),
};
enum class OPCODE_M : opcode_type {
    Make_entry(SM),
    Make_entry(LM),
    Make_entry(AA),
    Make_entry(AD),
    Make_entry(PTR),
};
#undef Make_entry
}  // namespace viua::arch::ops

#endif
