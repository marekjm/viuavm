/*
 *  Copyright (C) 2021-2023 Marek Marecki
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

#include <endian.h>
#include <stdint.h>
#include <string.h>

#include <stdexcept>
#include <string>
#include <string_view>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


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
    case FORMAT::M:
        return "M";
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
    case OPCODE::ECALL:
        return greedy + "ecall";
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
    case OPCODE::FRAME:
        return greedy + "frame";
    case OPCODE::LUI:
        return greedy + "lui";
    case OPCODE::LUIU:
        return greedy + "luiu";
    case OPCODE::LLI:
        return greedy + "lli";
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
    case OPCODE::IF:
        return greedy + "if";
    case OPCODE::IO_SUBMIT:
        return greedy + "io_submit";
    case OPCODE::IO_WAIT:
        return greedy + "io_wait";
    case OPCODE::IO_SHUTDOWN:
        return greedy + "io_shutdown";
    case OPCODE::IO_CTL:
        return greedy + "io_ctl";
    case OPCODE::IO_PEEK:
        return greedy + "io_peek";
    case OPCODE::ACTOR:
        return greedy + "actor";
    case OPCODE::SELF:
        return greedy + "self";
    case OPCODE::GTS:
        return greedy + "gts";
    case OPCODE::GTL:
        return greedy + "gtl";
    case OPCODE::CAST:
        return greedy + "cast";
    case OPCODE::ARODP:
        return greedy + "arodp";
    case OPCODE::ATXTP:
        return greedy + "atxtp";
    case OPCODE::SM:
        return greedy + "sm";
    case OPCODE::LM:
        return greedy + "lm";
    case OPCODE::AA:
        return greedy + "ama";
    case OPCODE::AD:
        return greedy + "amd";
    case OPCODE::PTR:
        return greedy + "ptr";
    }

    return "<unknown>";
}
auto parse_opcode(std::string_view const raw) -> opcode_type
{
    auto sv = raw;

    auto const greedy = sv.starts_with("g.");
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
    } else if (sv == "ecall") {
        return (op | static_cast<opcode_type>(OPCODE::ECALL));
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
    } else if (sv == "frame") {
        return (op | static_cast<opcode_type>(OPCODE::FRAME));
    } else if (sv == "lui") {
        return (op | static_cast<opcode_type>(OPCODE::LUI));
    } else if (sv == "luiu") {
        return (op | static_cast<opcode_type>(OPCODE::LUIU));
    } else if (sv == "lli") {
        return (op | static_cast<opcode_type>(OPCODE::LLI));
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
    } else if (sv == "if") {
        return (op | static_cast<opcode_type>(OPCODE::IF));
    } else if (sv == "io_submit") {
        return (op | static_cast<opcode_type>(OPCODE::IO_SUBMIT));
    } else if (sv == "io_wait") {
        return (op | static_cast<opcode_type>(OPCODE::IO_WAIT));
    } else if (sv == "io_shutdown") {
        return (op | static_cast<opcode_type>(OPCODE::IO_SHUTDOWN));
    } else if (sv == "io_ctl") {
        return (op | static_cast<opcode_type>(OPCODE::IO_CTL));
    } else if (sv == "io_peek") {
        return (op | static_cast<opcode_type>(OPCODE::IO_PEEK));
    } else if (sv == "actor") {
        return (op | static_cast<opcode_type>(OPCODE::ACTOR));
    } else if (sv == "self") {
        return (op | static_cast<opcode_type>(OPCODE::SELF));
    } else if (sv == "gts") {
        return (op | static_cast<opcode_type>(OPCODE::GTS));
    } else if (sv == "gtl") {
        return (op | static_cast<opcode_type>(OPCODE::GTL));
    } else if (sv == "cast") {
        return (op | static_cast<opcode_type>(OPCODE::CAST));
    } else if (sv == "arodp") {
        return (op | static_cast<opcode_type>(OPCODE::ARODP));
    } else if (sv == "atxtp") {
        return (op | static_cast<opcode_type>(OPCODE::ATXTP));
    } else if (sv == "sm") {
        return (op | static_cast<opcode_type>(OPCODE::SM));
    } else if (sv == "lm") {
        return (op | static_cast<opcode_type>(OPCODE::LM));
    } else if (sv == "ama") {
        return (op | static_cast<opcode_type>(OPCODE::AA));
    } else if (sv == "amd") {
        return (op | static_cast<opcode_type>(OPCODE::AD));
    } else if (sv == "ptr") {
        return (op | static_cast<opcode_type>(OPCODE::PTR));
    } else {
        throw std::invalid_argument{"viua::arch::ops::parse_opcode: "
                                    + std::string{raw}};
    }
}
}  // namespace viua::arch::ops
