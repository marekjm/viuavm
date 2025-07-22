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
constexpr auto FORMAT_N = opcode_type{ 0x00'00 };
constexpr auto FORMAT_T = opcode_type{ 0x10'00 };
constexpr auto FORMAT_D = opcode_type{ 0x20'00 };
constexpr auto FORMAT_S = opcode_type{ 0x30'00 };
constexpr auto FORMAT_F = opcode_type{ 0x40'00 };
constexpr auto FORMAT_E = opcode_type{ 0x50'00 };
constexpr auto FORMAT_R = opcode_type{ 0x60'00 };
constexpr auto FORMAT_M = opcode_type{ 0x70'00 };

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
enum class FORMAT : opcode_type
{
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
    static auto make(
        viua::arch::opcode_type const op,
        Register_access const o,
        T const v) -> F
    {
        static_assert(sizeof(T) == sizeof(uint32_t));
        auto imm = uint32_t{};
        memcpy(&imm, &v, sizeof(imm));
        return F{ op, o, imm };
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

constexpr auto GREEDY      = opcode_type{ 0x80'00 };
constexpr auto UNSIGNED    = opcode_type{ 0x08'00 };
constexpr auto INSTR_MASK  = opcode_type{ 0x0f'ff };
constexpr auto FORMAT_MASK = opcode_type{ 0x70'00 };
constexpr auto OPCODE_MASK = opcode_type{ FORMAT_MASK | INSTR_MASK };

enum class OPCODE : opcode_type
{
    NOOP   = (FORMAT_N | 0x00'00),
    HALT   = (FORMAT_N | 0x00'01),
    EBREAK = (FORMAT_N | 0x00'02),
    ECALL  = (FORMAT_N | 0x00'03),

    ADD         = (FORMAT_T | 0x00'01),
    SUB         = (FORMAT_T | 0x00'02),
    MUL         = (FORMAT_T | 0x00'03),
    DIV         = (FORMAT_T | 0x00'04),
    MOD         = (FORMAT_T | 0x00'05),
    BITSHL      = (FORMAT_T | 0x00'06),
    BITSHR      = (FORMAT_T | 0x00'07),
    BITASHR     = (FORMAT_T | 0x00'08),
    BITROL      = (FORMAT_T | 0x00'09),
    BITROR      = (FORMAT_T | 0x00'0a),
    BITAND      = (FORMAT_T | 0x00'0b),
    BITOR       = (FORMAT_T | 0x00'0c),
    BITXOR      = (FORMAT_T | 0x00'0d),
    EQ          = (FORMAT_T | 0x00'0e),
    LT          = (FORMAT_T | 0x00'0f),
    GT          = (FORMAT_T | 0x00'10),
    CMP         = (FORMAT_T | 0x00'11),
    AND         = (FORMAT_T | 0x00'12),
    OR          = (FORMAT_T | 0x00'13),
    IO_SUBMIT   = (FORMAT_T | 0x00'14),
    IO_WAIT     = (FORMAT_T | 0x00'15),
    IO_SHUTDOWN = (FORMAT_T | 0x00'16),
    IO_CTL      = (FORMAT_T | 0x00'17),

    CALL    = (FORMAT_D | 0x00'01),
    BITNOT  = (FORMAT_D | 0x00'02),
    NOT     = (FORMAT_D | 0x00'03),
    COPY    = (FORMAT_D | 0x00'04),
    MOVE    = (FORMAT_D | 0x00'05),
    SWAP    = (FORMAT_D | 0x00'06),
    IF      = (FORMAT_D | 0x00'07),
    IO_PEEK = (FORMAT_D | 0x00'08),
    ACTOR   = (FORMAT_D | 0x00'09),
    GTS     = (FORMAT_D | 0x00'0a),
    GTL     = (FORMAT_D | 0x00'0b),

    FRAME  = (FORMAT_S | 0x00'01),
    RETURN = (FORMAT_S | 0x00'02),
    ATOM   = (FORMAT_S | 0x00'03),
    DOUBLE = (FORMAT_S | 0x00'04),
    SELF   = (FORMAT_S | 0x00'05),

    LUI   = (FORMAT_F | 0x00'01),
    LUIU  = (FORMAT_F | 0x00'01 | UNSIGNED),
    LLI   = (FORMAT_F | 0x00'02),
    FLOAT = (FORMAT_F | 0x00'03),

    CAST  = (FORMAT_E | 0x00'01),
    ARODP = (FORMAT_E | 0x00'02),
    ATXTP = (FORMAT_E | 0x00'03),

    ADDI  = (FORMAT_R | 0x00'01),
    ADDIU = (FORMAT_R | 0x00'01 | UNSIGNED),
    SUBI  = (FORMAT_R | 0x00'02),
    SUBIU = (FORMAT_R | 0x00'02 | UNSIGNED),
    MULI  = (FORMAT_R | 0x00'03),
    MULIU = (FORMAT_R | 0x00'03 | UNSIGNED),
    DIVI  = (FORMAT_R | 0x00'04),
    DIVIU = (FORMAT_R | 0x00'04 | UNSIGNED),

    SM  = (FORMAT_M | 0x00'01), /* Store Memory */
    LM  = (FORMAT_M | 0x00'02), /* Load Memory */
    AA  = (FORMAT_M | 0x00'03), /* Allocate Automatic */
    AD  = (FORMAT_M | 0x00'04), /* Allocate Dynamic */
    PTR = (FORMAT_M | 0x00'05), /* PoinTeR */
};
auto to_string(opcode_type const) -> std::string;
auto parse_opcode(std::string_view) -> opcode_type;

/*
 * These are helper enums to provide exhaustiveness checks for switch
 * statements on opcodes of one format.
 */
#define Make_entry(OP) OP = static_cast<opcode_type>(OPCODE::OP)
enum class OPCODE_T : opcode_type
{
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
    Make_entry(IO_SUBMIT),
    Make_entry(IO_WAIT),
    Make_entry(IO_SHUTDOWN),
    Make_entry(IO_CTL),
};
enum class OPCODE_D : opcode_type
{
    Make_entry(CALL),
    Make_entry(BITNOT),
    Make_entry(NOT),
    Make_entry(COPY),
    Make_entry(MOVE),
    Make_entry(SWAP),
    Make_entry(IF),
    Make_entry(IO_PEEK),
    Make_entry(ACTOR),
    Make_entry(GTS),
    Make_entry(GTL),
};
enum class OPCODE_S : opcode_type
{
    Make_entry(FRAME),
    Make_entry(RETURN),
    Make_entry(ATOM),
    Make_entry(DOUBLE),
    Make_entry(SELF),
};
enum class OPCODE_F : opcode_type
{
    Make_entry(LUI),
    Make_entry(LUIU),
    Make_entry(LLI),
    Make_entry(FLOAT),
};
enum class OPCODE_E : opcode_type
{
    Make_entry(CAST),
    Make_entry(ARODP),
    Make_entry(ATXTP),
};
enum class OPCODE_R : opcode_type
{
    Make_entry(ADDI),
    Make_entry(ADDIU),
    Make_entry(SUBI),
    Make_entry(SUBIU),
    Make_entry(MULI),
    Make_entry(MULIU),
    Make_entry(DIVI),
    Make_entry(DIVIU),
};
enum class OPCODE_N : opcode_type
{
    Make_entry(NOOP),
    Make_entry(HALT),
    Make_entry(EBREAK),
    Make_entry(ECALL),
};
enum class OPCODE_M : opcode_type
{
    Make_entry(SM),
    Make_entry(LM),
    Make_entry(AA),
    Make_entry(AD),
    Make_entry(PTR),
};
#undef Make_entry
}  // namespace viua::arch::ops

namespace viua {
template<typename Sub, size_t Offset, typename T>
constexpr auto carve_bits_out(
    T const v) -> Sub
{
    constexpr auto mask = static_cast<T>(static_cast<Sub>(-1)) << Offset;
    return static_cast<Sub>((v & mask) >> Offset);
}

constexpr inline auto carve_opcode_out(
    viua::arch::instruction_type const i) -> viua::arch::opcode_type
{
    return carve_bits_out<viua::arch::opcode_type, 48>(i);
}

constexpr inline auto carve_just_opcode_out(
    viua::arch::instruction_type const i) -> viua::arch::ops::OPCODE
{
    return static_cast<viua::arch::ops::OPCODE>(carve_opcode_out(i)
                                                & viua::arch::ops::OPCODE_MASK);
}

constexpr inline auto carve_format_out(
    viua::arch::opcode_type const o) -> viua::arch::ops::FORMAT
{
    return static_cast<viua::arch::ops::FORMAT>(o
                                                & viua::arch::ops::FORMAT_MASK);
}
constexpr inline auto carve_format_out(
    viua::arch::ops::OPCODE const o) -> viua::arch::ops::FORMAT
{
    return carve_format_out(static_cast<viua::arch::opcode_type>(o));
}
}  // namespace viua

#endif
