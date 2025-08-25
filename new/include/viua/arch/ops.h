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
constexpr auto FORMAT_N = opcode_type{ 0b000 << 13 };
constexpr auto FORMAT_T = opcode_type{ 0b001 << 13 };
constexpr auto FORMAT_D = opcode_type{ 0b010 << 13 };
constexpr auto FORMAT_S = opcode_type{ 0b011 << 13 };
constexpr auto FORMAT_I = opcode_type{ 0b100 << 13 };
constexpr auto FORMAT_U = opcode_type{ 0b101 << 13 };
constexpr auto FORMAT_M = opcode_type{ 0b110 << 13 };

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
    M = FORMAT_M,
    I = FORMAT_I,
    U = FORMAT_U,
};
auto to_string(FORMAT const) -> std::string;

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
 * Two-way register access, with 32-bit memory offset specifier.
 * "M" because it is used for loads and stored, which interact with "memory".
 */
struct M {
    viua::arch::opcode_type opcode;
    Register_access const out;
    Register_access const in;
    uint32_t const immediate;

    M(viua::arch::opcode_type const,
      Register_access const,
      Register_access const,
      uint32_t const);

    static auto decode(instruction_type const) -> M;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;

    auto get_spec() const -> opcode_type;
    auto get_shift_size() const -> size_t;
};

/*
 * One-way register access with 32-bit wide immediate value.
 * "I" because it is the primary format with an "immediate" value.
 */
struct I {
    viua::arch::opcode_type opcode;
    Register_access const out;
    uint32_t const immediate;

    I(viua::arch::opcode_type const op,
      Register_access const o,
      uint32_t const i);

    template<typename T>
    static auto make(
        viua::arch::opcode_type const op,
        Register_access const o,
        T const v) -> I
    {
        static_assert(sizeof(T) == sizeof(uint32_t));
        auto imm = uint32_t{};
        memcpy(&imm, &v, sizeof(imm));
        return I{ op, o, imm };
    }

    static auto decode(instruction_type const) -> I;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

/*
 * Two-way register access with 32-bit wide immediate value.
 * "U" because it is a "useful" format.
 */
struct U {
    viua::arch::opcode_type opcode;
    Register_access const out;
    Register_access const in;
    uint32_t const immediate;

    U(viua::arch::opcode_type const,
      Register_access const,
      Register_access const,
      uint32_t const);

    static auto decode(instruction_type const) -> U;
    auto encode() const -> instruction_type;

    auto to_string() const -> std::string;
};

constexpr auto OPCODE_FMT_MASK = opcode_type{ 0b1110'0000'0000'0000 };
constexpr auto OPCODE_FLG_MASK = opcode_type{ 0b0001'1100'0000'0000 };
constexpr auto OPCODE_OPR_MASK = opcode_type{ 0b0000'0011'1111'1111 };
constexpr auto OPCODE_OPC_MASK =
    opcode_type{ OPCODE_FMT_MASK | OPCODE_OPR_MASK };

/*
 * How about something different:
 *
 *      OPCODE_FMT_MASK     e0'00   1110'0000'0000'0000
 *      FLAGS_MASK      1e'00   0001'1110'0000'0000
 *      OPCODE_MASK     11'ff   0001'0001'1111'1111
 *      INSTR_MASK      1f'ff   0001'1111'1111'1111
 *
 * CPU would only dispatch on OPCODE_MASK bits, which looks like we only have 10
 * bits available for instructions. However, it is 10 bits time three bits for
 * every format, so actually 13 bits.
 *
 * Given that flags are only relevant to M-format we could also count those bits
 * for every other instruction format to gain even more bits.
 *
 * Memory instructions would have to be a little bit different because of the
 * unit flags, but other instructions do not need flags... except for maybe the
 * "styled arithmetic" instructions with their styles, but the units would not
 * work there: styled arithmetic can use single bit resolution widths, which is
 * a much finer level of control than the word resolution used by memory
 * instructions.
 *
 * Maybe have an 8-bit ememoryunit register to specify the unit for memory
 * instructions, and a group of E[LSM]M instructions--Environment
 * Load|Store|Move Memory--that would take their unit from that register:
 *
 *      ; Load Byte Immediate
 *      lbi void, $src.l, <offset>
 *
 *      ; Load Byte
 *      lb void, $src.l, $offset.l
 *
 *      ; Environment Load Memory Immediate
 *      elmi void, $src.l, <offset>
 *
 *      ; Environment Load Memory
 *      elm void, $src.l, $offset.l
 *
 * Having the ExM instructions means that only the most useful units need to be
 * encoded directly into the instruction ie, ones that represent units up to the
 * width of a register; which is 64 bits (16 bytes) ie, a quad-word. For such a
 * design we need three bits for the flags, so we may as well go up to the
 * duotrigesimal word. (This would also bring us to a nice, round 1024-bit wide
 * register, which could be useful for page tables with 1024 entries.)
 *
 * But wait, there is more! Having whole three bits for flags means that we
 * could actually embed the arithmetic style into an instruction! So
 *
 *      earithmeticstyle void, <saturating>
 *      stdadd ...
 *
 * can become
 *
 *      add.saturate ...
 *
 * or even (if we go the attributes route)
 *
 *      add [[style=saturate]] ...
 *
 * There is no space to encode all the possible widths of the integer, but we
 * can leave that in an environment register. (As we would for the vector
 * registers.)
 *
 * Loading vector registers would be done with the usual memory instructions,
 * but could be made incredibly easy by the ELM instruction: everything could be
 * detected and configured at runtime, and nothing would be hardcoded.
 *
 *      ; Notice the similarity to stdadd. The vecxyz instructions would use the
 *      ; earithmeticwidth register for the size of their elements.
 *      vecadd [[style=saturate]] ...
 *
 * I think the design is shaping up nicely, after all.
 */

namespace OPCODE_FLAGS {
constexpr auto FLAGS_SHIFT = size_t{ 10 };

/*
 * Used for memory operations (instructions in M format).
 *
 * The SM, LM, MM, and AA instructions are all encoded on the lowest nibble of
 * the opcode. The highest nibble is reserved for the format and the unsigned
 * flag, but the second nibble could be used to encode the unit of the memory
 * instruction.
 *
 * See https://www.numberbases.com/terms/basename1.html for the origin of the
 * "duotrigesimal" name and the UNIT_DUOTRI_WORD flag.
 */
constexpr auto UNIT_BYTE        = opcode_type{ 0b000 << FLAGS_SHIFT };
constexpr auto UNIT_HALF_WORD   = opcode_type{ 0b001 << FLAGS_SHIFT };
constexpr auto UNIT_WORD        = opcode_type{ 0b010 << FLAGS_SHIFT };
constexpr auto UNIT_DOUBLE_WORD = opcode_type{ 0b011 << FLAGS_SHIFT };
constexpr auto UNIT_QUAD_WORD   = opcode_type{ 0b100 << FLAGS_SHIFT };
constexpr auto UNIT_OCTA_WORD   = opcode_type{ 0b101 << FLAGS_SHIFT };
constexpr auto UNIT_HEXA_WORD   = opcode_type{ 0b110 << FLAGS_SHIFT };
constexpr auto UNIT_DUOTRI_WORD = opcode_type{ 0b111 << FLAGS_SHIFT };

/*
 * Used for arithmetic instructions.
 */
constexpr auto ARITHMETIC_STYLE_NATIVE   = opcode_type{ 0b000 << FLAGS_SHIFT };
constexpr auto ARITHMETIC_STYLE_WRAP     = opcode_type{ 0b001 << FLAGS_SHIFT };
constexpr auto ARITHMETIC_STYLE_SATURATE = opcode_type{ 0b010 << FLAGS_SHIFT };
constexpr auto ARITHMETIC_STYLE_TRAP     = opcode_type{ 0b011 << FLAGS_SHIFT };
/*
 * Unsigned MUST NOT conflict with any of the other arithmetic flags.
 *
 * The unsigned flag is used to denote instructions constructing unsigned
 * values, but is actually irrelevant for arithmetic, since the arithmetic
 * instructions' output type depends on the type of their left-hand side
 * operand.
 *
 * The flag is necessary for constructors because the VM must have some way of
 * creating an unsigned value, without being able to construct unsigned integers
 * the lhs-derived type trickery would not be possible.
 *
 * But. This means that the flag is not really a flag, and could be rolled into
 * the operation mask proper ie, into the lowest 9 bits. If it were, the bit
 * reserved for it would be freed and the opcode encoding could be changed to:
 *
 *  - 3 bits for the format
 *  - 3 bits for the flags
 *  - 10 bits for the operation
 *
 * I think this is reasonable. What is more, I think that the only two
 * instructions that have to be explicitly-unsigned are LUIU and ADDIU, as they
 * are the means of loading unsigned and 64-bit values. But the SUBIU, MULIU,
 * and DIVIU? They could all be replaced be a combination of ADDIU and the
 * non-unsigned variant; for example
 *
 *      subiu a, b, 42
 *
 * might be written as:
 *
 *      addiu c, void, 42
 *      sub a, b, c
 *
 * The same transformation is also valid for multiplication and division.
 */
constexpr auto UNSIGNED = opcode_type{ 0x02'00 };
}  // namespace OPCODE_FLAGS

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

    CALL             = (FORMAT_D | 0x00'01),
    BITNOT           = (FORMAT_D | 0x00'02),
    NOT              = (FORMAT_D | 0x00'03),
    COPY             = (FORMAT_D | 0x00'04),
    MOVE             = (FORMAT_D | 0x00'05),
    SWAP             = (FORMAT_D | 0x00'06),
    IF               = (FORMAT_D | 0x00'07),
    IO_PEEK          = (FORMAT_D | 0x00'08),
    ACTOR            = (FORMAT_D | 0x00'09),
    GTS              = (FORMAT_D | 0x00'0a),
    GTL              = (FORMAT_D | 0x00'0b),
    EARITHMETICWIDTH = (FORMAT_D | 0x00'0c),
    BITREV           = (FORMAT_D | 0x00'0d),
    BITAREV          = (FORMAT_D | 0x00'0e),

    FRAME  = (FORMAT_S | 0x00'01),
    RETURN = (FORMAT_S | 0x00'02),
    ATOM   = (FORMAT_S | 0x00'03),
    DOUBLE = (FORMAT_S | 0x00'04),
    SELF   = (FORMAT_S | 0x00'05),

    LUI  = (FORMAT_I | 0x00'01),
    LUIU = (FORMAT_I | LUI | OPCODE_FLAGS::UNSIGNED),
    // FIXME remove the lli instruction, use addi/addiu instead
    LLI   = (FORMAT_I | 0x00'02),
    FLOAT = (FORMAT_I | 0x00'03),
    CAST  = (FORMAT_I | 0x00'04),
    ARODP = (FORMAT_I | 0x00'05),
    ATXTP = (FORMAT_I | 0x00'06),

    ADDI  = (FORMAT_U | 0x00'01),
    ADDIU = (FORMAT_U | ADDI | OPCODE_FLAGS::UNSIGNED),
    SUBI  = (FORMAT_U | 0x00'02),
    SUBIU = (FORMAT_U | SUBI | OPCODE_FLAGS::UNSIGNED),
    MULI  = (FORMAT_U | 0x00'03),
    MULIU = (FORMAT_U | MULI | OPCODE_FLAGS::UNSIGNED),
    DIVI  = (FORMAT_U | 0x00'04),
    DIVIU = (FORMAT_U | DIVI | OPCODE_FLAGS::UNSIGNED),

    SM = (FORMAT_M | 0x00'01), /* Store Memory */
    // SMI =
    LM = (FORMAT_M | 0x00'02), /* Load Memory */
    // LMI
    // MM  = (FORMAT_M | 0x00'03), /* Move Memory */
    AA  = (FORMAT_M | 0x00'04), /* Allocate Automatic */
    AD  = (FORMAT_M | 0x00'05), /* Allocate Dynamic */
    PTR = (FORMAT_M | 0x00'07), /* PoinTeR */
};
auto to_string(opcode_type const) -> std::string;
auto parse_opcode(std::string_view) -> opcode_type;
auto is_format(FORMAT const, opcode_type const) -> bool;
auto is_format(FORMAT const, OPCODE const) -> bool;

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
    Make_entry(EARITHMETICWIDTH),
    Make_entry(BITREV),
    Make_entry(BITAREV),
};
enum class OPCODE_S : opcode_type
{
    Make_entry(FRAME),
    Make_entry(RETURN),
    Make_entry(ATOM),
    Make_entry(DOUBLE),
    Make_entry(SELF),
};
enum class OPCODE_I : opcode_type
{
    Make_entry(LUI),
    Make_entry(LUIU),
    Make_entry(LLI),
    Make_entry(FLOAT),
    Make_entry(CAST),
    Make_entry(ARODP),
    Make_entry(ATXTP),
};
enum class OPCODE_U : opcode_type
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
    return carve_bits_out<viua::arch::opcode_type, 0>(i);
}

constexpr inline auto carve_just_opcode_out(
    viua::arch::instruction_type const i) -> viua::arch::ops::OPCODE
{
    return static_cast<viua::arch::ops::OPCODE>(
        carve_opcode_out(i) & viua::arch::ops::OPCODE_OPC_MASK);
}

constexpr inline auto carve_format_out(
    viua::arch::opcode_type const o) -> viua::arch::ops::FORMAT
{
    return static_cast<viua::arch::ops::FORMAT>(
        o & viua::arch::ops::OPCODE_FMT_MASK);
}
constexpr inline auto carve_format_out(
    viua::arch::ops::OPCODE const o) -> viua::arch::ops::FORMAT
{
    return carve_format_out(static_cast<viua::arch::opcode_type>(o));
}

constexpr inline auto carve_flags_out(
    viua::arch::opcode_type const o) -> viua::arch::opcode_type
{
    return static_cast<viua::arch::opcode_type>(
        o & viua::arch::ops::OPCODE_FLG_MASK);
}
constexpr inline auto carve_flags_out(
    viua::arch::ops::OPCODE const o) -> viua::arch::opcode_type
{
    return carve_flags_out(static_cast<viua::arch::opcode_type>(o));
}

struct compose_filler {
    size_t const size;
};

template<typename Into, typename Last>
constexpr auto compose_bits_into_impl(
    Into const accumulator,
    size_t const offset,
    Last const last) -> Into
{
    if ((sizeof(Into) * 8) != (offset + (sizeof(Last) * 8))) {
        throw std::logic_error{
            "compose_bits_into: parts do not fill the output type"
        };
    }
    return accumulator | (static_cast<Into>(last) << offset);
}
template<typename Into>
constexpr auto compose_bits_into_impl(
    Into const accumulator,
    size_t const offset,
    compose_filler const last) -> Into
{
    if ((sizeof(Into) * 8) != (offset + last.size)) {
        throw std::logic_error{
            "compose_bits_into: parts do not fill the output type"
        };
    }
    return accumulator;
}
template<typename Into, typename... Args>
constexpr auto compose_bits_into_impl(
    Into const accumulator,
    size_t const offset,
    compose_filler const filler,
    Args const... args) -> Into
{
    return compose_bits_into_impl(accumulator, offset + filler.size, args...);
}
template<typename Into, typename First, typename... Args>
constexpr auto compose_bits_into_impl(
    Into const accumulator,
    size_t const offset,
    First const first,
    Args const... args) -> Into
{
    return compose_bits_into_impl(
        accumulator | (static_cast<Into>(first) << offset),
        offset + (sizeof(First) * 8),
        args...);
}

template<typename Into, typename... Args>
constexpr auto compose_bits_into(
    Args const... args) -> Into
{
    return compose_bits_into_impl(Into{}, 0, args...);
}
}  // namespace viua

#endif
