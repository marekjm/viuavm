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
    case OPCODE::CALL:
        return greedy + "call";
    case OPCODE::DELETE:
        return greedy + "delete";
    case OPCODE::STRING:
        return greedy + "string";
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
    } else if (sv == "call") {
        return (op | static_cast<opcode_type>(OPCODE::CALL));
    } else if (sv == "delete") {
        return (op | static_cast<opcode_type>(OPCODE::DELETE));
    } else if (sv == "string") {
        return (op | static_cast<opcode_type>(OPCODE::STRING));
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
    } else {
        throw std::invalid_argument{"viua::arch::ops::parse_opcode: "
                                    + std::string{raw}};
    }
}
}  // namespace viua::arch::ops