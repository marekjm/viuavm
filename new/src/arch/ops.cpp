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

#include <stdexcept>
#include <string>
#include <string_view>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


namespace viua::arch::ops {
T::T(viua::arch::opcode_type const op,
     Register_access const o,
     Register_access const l,
     Register_access const r)
        : opcode{op}, out{o}, lhs{l}, rhs{r}
{}
auto T::decode(instruction_type const raw) -> T
{
    auto opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto lhs = Register_access::decode((raw & 0x0000ffff00000000) >> 32);
    auto rhs = Register_access::decode((raw & 0xffff000000000000) >> 48);
    return T{opcode, out, lhs, rhs};
}
auto T::encode() const -> instruction_type
{
    auto base                = uint64_t{opcode};
    auto output_register     = uint64_t{out.encode()};
    auto left_hand_register  = uint64_t{lhs.encode()};
    auto right_hand_register = uint64_t{rhs.encode()};
    return base | (output_register << 16) | (left_hand_register << 32)
           | (right_hand_register << 48);
}
}  // namespace viua::arch::ops
namespace viua::arch::ops {
D::D(viua::arch::opcode_type const op,
     Register_access const o,
     Register_access const i)
        : opcode{op}, out{o}, in{i}
{}
auto D::decode(instruction_type const raw) -> D
{
    auto opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto in  = Register_access::decode((raw & 0x0000ffff00000000) >> 32);
    return D{opcode, out, in};
}
auto D::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto input_register  = uint64_t{in.encode()};
    return base | (output_register << 16) | (input_register << 32);
}
}  // namespace viua::arch::ops
namespace viua::arch::ops {
S::S(viua::arch::opcode_type const op, Register_access const o)
        : opcode{op}, out{o}
{}
auto S::decode(instruction_type const raw) -> S
{
    auto opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    return S{opcode, out};
}
auto S::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    return base | (output_register << 16);
}
}  // namespace viua::arch::ops
namespace viua::arch::ops {
F::F(viua::arch::opcode_type const op,
     Register_access const o,
     uint32_t const i)
        : opcode{op}, out{o}, immediate{i}
{}
auto F::decode(instruction_type const raw) -> F
{
    auto opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto out   = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto value = static_cast<uint32_t>(raw >> 32);
    return F{opcode, out, value};
}
auto F::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto value           = uint64_t{immediate};
    return base | (output_register << 16) | (value << 32);
}
}  // namespace viua::arch::ops
namespace viua::arch::ops {
E::E(viua::arch::opcode_type const op,
     Register_access const o,
     uint64_t const i)
        : opcode{op}, out{o}, immediate{i}
{}
auto E::decode(instruction_type const raw) -> E
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto const out  = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto const high = (((raw >> 28) & 0xf) << 32);
    auto const low  = ((raw >> 32) & 0x00000000ffffffff);
    auto const value = (high | low);
    return E{opcode, out, value};
}
auto E::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto high            = ((immediate & 0x0000000f00000000) >> 32);
    auto low             = (immediate & 0x00000000ffffffff);
    return base | (output_register << 16) | (high << 28) | (low << 32);
}
}  // namespace viua::arch::ops
namespace viua::arch::ops {
R::R(viua::arch::opcode_type const op,
     Register_access const o,
     Register_access const i,
     uint32_t const im)
        : opcode{op}, out{o}, in{i}, immediate{im}
{}
auto R::decode(instruction_type const raw) -> R
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
    auto const out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
    auto const in  = Register_access::decode((raw & 0x0000ffff00000000) >> 32);

    auto const low_short =
        static_cast<uint32_t>((raw & 0xffff000000000000) >> 48);
    auto const low_nibble =
        static_cast<uint32_t>((raw & 0x0000f00000000000) >> 44);
    auto const high_nibble =
        static_cast<uint32_t>((raw & 0x00000000f0000000) >> 28);

    auto const immediate = low_short | (low_nibble << 16) | (high_nibble << 20);

    return R{opcode, out, in, immediate};
}
auto R::encode() const -> instruction_type
{
    auto base            = uint64_t{opcode};
    auto output_register = uint64_t{out.encode()};
    auto input_register  = uint64_t{in.encode()};

    auto const high_nibble = uint64_t{(immediate & 0x00f00000) >> 20};
    auto const low_nibble  = uint64_t{(immediate & 0x000f0000) >> 16};
    auto const low_short   = uint64_t{(immediate & 0x0000ffff) >> 0};

    return base | (output_register << 16) | (input_register << 32)
           | (low_short << 48) | (high_nibble << 28) | (low_nibble << 44);
}
}  // namespace viua::arch::ops
namespace viua::arch::ops {
N::N(viua::arch::opcode_type const op) : opcode{op}
{}
auto N::decode(instruction_type const raw) -> N
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);

    return N{opcode};
}
auto N::encode() const -> instruction_type
{
    return uint64_t{opcode};
}
}  // namespace viua::arch::ops

namespace viua::arch::ops {
auto to_string(FORMAT const raw) -> std::string
{
    switch (raw) {
    case FORMAT::N:
        return "N";
    case FORMAT::T:
        return "T";
    case FORMAT::D:
        return "D";
    case FORMAT::S:
        return "S";
    case FORMAT::F:
        return "F";
    case FORMAT::E:
        return "E";
    case FORMAT::R:
        return "R";
    }

    return "<unknown>";
}

auto to_string(opcode_type const raw) -> std::string
{
    auto const greedy =
        std::string{static_cast<bool>(raw & GREEDY) ? "g." : ""};
    auto const opcode = static_cast<OPCODE>(raw & OPCODE_MASK);

    switch (opcode) {
    case OPCODE::NOOP:
        return greedy + "noop";
    case OPCODE::HALT:
        return greedy + "halt";
    case OPCODE::EBREAK:
        return greedy + "ebreak";
    case OPCODE::RETURN:
        return greedy + "return";
    case OPCODE::ADD:
        return greedy + "add";
    case OPCODE::SUB:
        return greedy + "sub";
    case OPCODE::MUL:
        return greedy + "mul";
    case OPCODE::DIV:
        return greedy + "div";
    case OPCODE::MOD:
        return greedy + "mod";
    case OPCODE::BITSHL:
        return greedy + "bitshl";
    case OPCODE::BITSHR:
        return greedy + "bitshr";
    case OPCODE::BITASHR:
        return greedy + "bitashr";
    case OPCODE::BITROL:
        return greedy + "bitrol";
    case OPCODE::BITROR:
        return greedy + "bitror";
    case OPCODE::BITAND:
        return greedy + "bitand";
    case OPCODE::BITOR:
        return greedy + "bitor";
    case OPCODE::BITXOR:
        return greedy + "bitxor";
    case OPCODE::EQ:
        return "eq";
    case OPCODE::LT:
        return "lt";
    case OPCODE::GT:
        return "gt";
    case OPCODE::CMP:
        return "cmp";
    case OPCODE::AND:
        return "and";
    case OPCODE::OR:
        return "or";
    case OPCODE::CALL:
        return "call";
    case OPCODE::BITNOT:
        return greedy + "bitnot";
    case OPCODE::NOT:
        return greedy + "not";
    case OPCODE::ATOM:
        return greedy + "atom";
    case OPCODE::STRING:
        return greedy + "string";
    case OPCODE::STRUCT:
        return greedy + "struct";
    case OPCODE::BUFFER:
        return greedy + "buffer";
    case OPCODE::FRAME:
        return greedy + "frame";
    case OPCODE::LUI:
        return greedy + "lui";
    case OPCODE::LUIU:
        return greedy + "luiu";
    case OPCODE::ADDI:
        return greedy + "addi";
    case OPCODE::ADDIU:
        return greedy + "addiu";
    case OPCODE::SUBI:
        return greedy + "subi";
    case OPCODE::SUBIU:
        return greedy + "subiu";
    case OPCODE::MULI:
        return greedy + "muli";
    case OPCODE::MULIU:
        return greedy + "muliu";
    case OPCODE::DIVI:
        return greedy + "divi";
    case OPCODE::DIVIU:
        return greedy + "diviu";
    case OPCODE::FLOAT:
        return greedy + "float";
    case OPCODE::DOUBLE:
        return greedy + "double";
    case OPCODE::COPY:
        return greedy + "copy";
    case OPCODE::MOVE:
        return greedy + "move";
    case OPCODE::SWAP:
        return greedy + "swap";
    }

    return "<unknown>";
}
auto parse_opcode(std::string_view const raw) -> opcode_type
{
    auto sv = raw;

    auto greedy = sv.starts_with("g.");
    if (greedy) {
        sv.remove_prefix(2);
    }

    auto op = (greedy ? GREEDY : opcode_type{});
    if (sv == "noop") {
        return (op | static_cast<opcode_type>(OPCODE::NOOP));
    } else if (sv == "halt") {
        return (op | static_cast<opcode_type>(OPCODE::HALT));
    } else if (sv == "ebreak") {
        return (op | static_cast<opcode_type>(OPCODE::EBREAK));
    } else if (sv == "return") {
        return (op | static_cast<opcode_type>(OPCODE::RETURN));
    } else if (sv == "add") {
        return (op | static_cast<opcode_type>(OPCODE::ADD));
    } else if (sv == "sub") {
        return (op | static_cast<opcode_type>(OPCODE::SUB));
    } else if (sv == "mul") {
        return (op | static_cast<opcode_type>(OPCODE::MUL));
    } else if (sv == "div") {
        return (op | static_cast<opcode_type>(OPCODE::DIV));
    } else if (sv == "mod") {
        return (op | static_cast<opcode_type>(OPCODE::MOD));
    } else if (sv == "bitshl") {
        return (op | static_cast<opcode_type>(OPCODE::BITSHL));
    } else if (sv == "bitshr") {
        return (op | static_cast<opcode_type>(OPCODE::BITSHR));
    } else if (sv == "bitashr") {
        return (op | static_cast<opcode_type>(OPCODE::BITASHR));
    } else if (sv == "bitrol") {
        return (op | static_cast<opcode_type>(OPCODE::BITROL));
    } else if (sv == "bitror") {
        return (op | static_cast<opcode_type>(OPCODE::BITROR));
    } else if (sv == "bitand") {
        return (op | static_cast<opcode_type>(OPCODE::BITAND));
    } else if (sv == "bitor") {
        return (op | static_cast<opcode_type>(OPCODE::BITOR));
    } else if (sv == "bitxor") {
        return (op | static_cast<opcode_type>(OPCODE::BITXOR));
    } else if (sv == "eq") {
        return (op | static_cast<opcode_type>(OPCODE::EQ));
    } else if (sv == "lt") {
        return (op | static_cast<opcode_type>(OPCODE::LT));
    } else if (sv == "gt") {
        return (op | static_cast<opcode_type>(OPCODE::GT));
    } else if (sv == "cmp") {
        return (op | static_cast<opcode_type>(OPCODE::CMP));
    } else if (sv == "and") {
        return (op | static_cast<opcode_type>(OPCODE::AND));
    } else if (sv == "or") {
        return (op | static_cast<opcode_type>(OPCODE::OR));
    } else if (sv == "call") {
        return static_cast<opcode_type>(OPCODE::CALL);
    } else if (sv == "bitnot") {
        return (op | static_cast<opcode_type>(OPCODE::BITNOT));
    } else if (sv == "not") {
        return (op | static_cast<opcode_type>(OPCODE::NOT));
    } else if (sv == "atom") {
        return (op | static_cast<opcode_type>(OPCODE::ATOM));
    } else if (sv == "string") {
        return (op | static_cast<opcode_type>(OPCODE::STRING));
    } else if (sv == "struct") {
        return (op | static_cast<opcode_type>(OPCODE::STRUCT));
    } else if (sv == "buffer") {
        return (op | static_cast<opcode_type>(OPCODE::BUFFER));
    } else if (sv == "frame") {
        return (op | static_cast<opcode_type>(OPCODE::FRAME));
    } else if (sv == "lui") {
        return (op | static_cast<opcode_type>(OPCODE::LUI));
    } else if (sv == "luiu") {
        return (op | static_cast<opcode_type>(OPCODE::LUIU));
    } else if (sv == "addi") {
        return (op | static_cast<opcode_type>(OPCODE::ADDI));
    } else if (sv == "addiu") {
        return (op | static_cast<opcode_type>(OPCODE::ADDIU));
    } else if (sv == "subi") {
        return (op | static_cast<opcode_type>(OPCODE::SUBI));
    } else if (sv == "subiu") {
        return (op | static_cast<opcode_type>(OPCODE::SUBIU));
    } else if (sv == "muli") {
        return (op | static_cast<opcode_type>(OPCODE::MULI));
    } else if (sv == "muliu") {
        return (op | static_cast<opcode_type>(OPCODE::MULIU));
    } else if (sv == "divi") {
        return (op | static_cast<opcode_type>(OPCODE::DIVI));
    } else if (sv == "diviu") {
        return (op | static_cast<opcode_type>(OPCODE::DIVIU));
    } else if (sv == "float") {
        return (op | static_cast<opcode_type>(OPCODE::FLOAT));
    } else if (sv == "double") {
        return (op | static_cast<opcode_type>(OPCODE::DOUBLE));
    } else if (sv == "copy") {
        return (op | static_cast<opcode_type>(OPCODE::COPY));
    } else if (sv == "move") {
        return (op | static_cast<opcode_type>(OPCODE::MOVE));
    } else if (sv == "swap") {
        return (op | static_cast<opcode_type>(OPCODE::SWAP));
    } else {
        throw std::invalid_argument{"viua::arch::ops::parse_opcode: "
                                    + std::string{raw}};
    }
}
}  // namespace viua::arch::ops
