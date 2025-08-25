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

#include <stdexcept>
#include <string>
#include <string_view>

#include <viua/arch/ops.h>

namespace viua::arch::ops {
auto to_string(
    FORMAT const raw) -> std::string
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
        case FORMAT::I:
            return "I";
        case FORMAT::U:
            return "U";
        case FORMAT::M:
            return "M";
    }

    return "<unknown format>";
}

namespace {
auto to_string_impl(
    opcode_type const opcode) -> std::string
{
    switch (static_cast<OPCODE>(opcode)) {
        case OPCODE::NOOP:
            return "noop";
        case OPCODE::HALT:
            return "halt";
        case OPCODE::EBREAK:
            return "ebreak";
        case OPCODE::ECALL:
            return "ecall";
        case OPCODE::RETURN:
            return "return";
        case OPCODE::ADD:
            return "add";
        case OPCODE::SUB:
            return "sub";
        case OPCODE::MUL:
            return "mul";
        case OPCODE::DIV:
            return "div";
        case OPCODE::MOD:
            return "mod";
        case OPCODE::BITSHL:
            return "bitshl";
        case OPCODE::BITSHR:
            return "bitshr";
        case OPCODE::BITASHR:
            return "bitashr";
        case OPCODE::BITROL:
            return "bitrol";
        case OPCODE::BITROR:
            return "bitror";
        case OPCODE::BITAND:
            return "bitand";
        case OPCODE::BITOR:
            return "bitor";
        case OPCODE::BITXOR:
            return "bitxor";
        case OPCODE::BITREV:
            return "bitrev";
        case OPCODE::BITAREV:
            return "bitarev";
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
            return "bitnot";
        case OPCODE::NOT:
            return "not";
        case OPCODE::ATOM:
            return "atom";
        case OPCODE::FRAME:
            return "frame";
        case OPCODE::LUI:
            return "lui";
        case OPCODE::LUIU:
            return "luiu";
        case OPCODE::LLI:
            return "lli";
        case OPCODE::ADDI:
            return "addi";
        case OPCODE::ADDIU:
            return "addiu";
        case OPCODE::SUBI:
            return "subi";
        case OPCODE::SUBIU:
            return "subiu";
        case OPCODE::MULI:
            return "muli";
        case OPCODE::MULIU:
            return "muliu";
        case OPCODE::DIVI:
            return "divi";
        case OPCODE::DIVIU:
            return "diviu";
        case OPCODE::FLOAT:
            return "float";
        case OPCODE::DOUBLE:
            return "double";
        case OPCODE::COPY:
            return "copy";
        case OPCODE::MOVE:
            return "move";
        case OPCODE::SWAP:
            return "swap";
        case OPCODE::IF:
            return "if";
        case OPCODE::IO_SUBMIT:
            return "io_submit";
        case OPCODE::IO_WAIT:
            return "io_wait";
        case OPCODE::IO_SHUTDOWN:
            return "io_shutdown";
        case OPCODE::IO_CTL:
            return "io_ctl";
        case OPCODE::IO_PEEK:
            return "io_peek";
        case OPCODE::ACTOR:
            return "actor";
        case OPCODE::SELF:
            return "self";
        case OPCODE::GTS:
            return "gts";
        case OPCODE::GTL:
            return "gtl";
        case OPCODE::EARITHMETICWIDTH:
            return "earithmeticwidth";
        case OPCODE::CAST:
            return "cast";
        case OPCODE::ARODP:
            return "arodp";
        case OPCODE::ATXTP:
            return "atxtp";
        case OPCODE::SM:
            return "sm";
        case OPCODE::LM:
            return "lm";
        case OPCODE::AA:
            return "ama";
        case OPCODE::AD:
            return "amd";
        case OPCODE::PTR:
            return "ptr";
    }
    return "<unknown opcode>";
}
}  // namespace

auto to_string(
    opcode_type const raw) -> std::string
{
    auto const opcode = (raw & OPCODE_OPC_MASK);
    return to_string_impl(static_cast<opcode_type>(opcode));
}
auto parse_opcode(
    std::string_view const raw) -> opcode_type
{
    auto sv = raw;

    if (sv == "noop") {
        return static_cast<opcode_type>(OPCODE::NOOP);
    } else if (sv == "halt") {
        return static_cast<opcode_type>(OPCODE::HALT);
    } else if (sv == "ebreak") {
        return static_cast<opcode_type>(OPCODE::EBREAK);
    } else if (sv == "ecall") {
        return static_cast<opcode_type>(OPCODE::ECALL);
    } else if (sv == "return") {
        return static_cast<opcode_type>(OPCODE::RETURN);
    } else if (sv == "add") {
        return static_cast<opcode_type>(OPCODE::ADD);
    } else if (sv == "sub") {
        return static_cast<opcode_type>(OPCODE::SUB);
    } else if (sv == "mul") {
        return static_cast<opcode_type>(OPCODE::MUL);
    } else if (sv == "div") {
        return static_cast<opcode_type>(OPCODE::DIV);
    } else if (sv == "mod") {
        return static_cast<opcode_type>(OPCODE::MOD);
    } else if (sv == "bitshl") {
        return static_cast<opcode_type>(OPCODE::BITSHL);
    } else if (sv == "bitshr") {
        return static_cast<opcode_type>(OPCODE::BITSHR);
    } else if (sv == "bitashr") {
        return static_cast<opcode_type>(OPCODE::BITASHR);
    } else if (sv == "bitrol") {
        return static_cast<opcode_type>(OPCODE::BITROL);
    } else if (sv == "bitror") {
        return static_cast<opcode_type>(OPCODE::BITROR);
    } else if (sv == "bitand") {
        return static_cast<opcode_type>(OPCODE::BITAND);
    } else if (sv == "bitor") {
        return static_cast<opcode_type>(OPCODE::BITOR);
    } else if (sv == "bitxor") {
        return static_cast<opcode_type>(OPCODE::BITXOR);
    } else if (sv == "bitrev") {
        return static_cast<opcode_type>(OPCODE::BITREV);
    } else if (sv == "bitarev") {
        return static_cast<opcode_type>(OPCODE::BITAREV);
    } else if (sv == "eq") {
        return static_cast<opcode_type>(OPCODE::EQ);
    } else if (sv == "lt") {
        return static_cast<opcode_type>(OPCODE::LT);
    } else if (sv == "gt") {
        return static_cast<opcode_type>(OPCODE::GT);
    } else if (sv == "cmp") {
        return static_cast<opcode_type>(OPCODE::CMP);
    } else if (sv == "and") {
        return static_cast<opcode_type>(OPCODE::AND);
    } else if (sv == "or") {
        return static_cast<opcode_type>(OPCODE::OR);
    } else if (sv == "call") {
        return static_cast<opcode_type>(OPCODE::CALL);
    } else if (sv == "bitnot") {
        return static_cast<opcode_type>(OPCODE::BITNOT);
    } else if (sv == "not") {
        return static_cast<opcode_type>(OPCODE::NOT);
    } else if (sv == "atom") {
        return static_cast<opcode_type>(OPCODE::ATOM);
    } else if (sv == "frame") {
        return static_cast<opcode_type>(OPCODE::FRAME);
    } else if (sv == "lui") {
        return static_cast<opcode_type>(OPCODE::LUI);
    } else if (sv == "luiu") {
        return static_cast<opcode_type>(OPCODE::LUIU);
    } else if (sv == "lli") {
        return static_cast<opcode_type>(OPCODE::LLI);
    } else if (sv == "addi") {
        return static_cast<opcode_type>(OPCODE::ADDI);
    } else if (sv == "addiu") {
        return static_cast<opcode_type>(OPCODE::ADDIU);
    } else if (sv == "subi") {
        return static_cast<opcode_type>(OPCODE::SUBI);
    } else if (sv == "subiu") {
        return static_cast<opcode_type>(OPCODE::SUBIU);
    } else if (sv == "muli") {
        return static_cast<opcode_type>(OPCODE::MULI);
    } else if (sv == "muliu") {
        return static_cast<opcode_type>(OPCODE::MULIU);
    } else if (sv == "divi") {
        return static_cast<opcode_type>(OPCODE::DIVI);
    } else if (sv == "diviu") {
        return static_cast<opcode_type>(OPCODE::DIVIU);
    } else if (sv == "float") {
        return static_cast<opcode_type>(OPCODE::FLOAT);
    } else if (sv == "double") {
        return static_cast<opcode_type>(OPCODE::DOUBLE);
    } else if (sv == "copy") {
        return static_cast<opcode_type>(OPCODE::COPY);
    } else if (sv == "move") {
        return static_cast<opcode_type>(OPCODE::MOVE);
    } else if (sv == "swap") {
        return static_cast<opcode_type>(OPCODE::SWAP);
    } else if (sv == "if") {
        return static_cast<opcode_type>(OPCODE::IF);
    } else if (sv == "io_submit") {
        return static_cast<opcode_type>(OPCODE::IO_SUBMIT);
    } else if (sv == "io_wait") {
        return static_cast<opcode_type>(OPCODE::IO_WAIT);
    } else if (sv == "io_shutdown") {
        return static_cast<opcode_type>(OPCODE::IO_SHUTDOWN);
    } else if (sv == "io_ctl") {
        return static_cast<opcode_type>(OPCODE::IO_CTL);
    } else if (sv == "io_peek") {
        return static_cast<opcode_type>(OPCODE::IO_PEEK);
    } else if (sv == "actor") {
        return static_cast<opcode_type>(OPCODE::ACTOR);
    } else if (sv == "self") {
        return static_cast<opcode_type>(OPCODE::SELF);
    } else if (sv == "gts") {
        return static_cast<opcode_type>(OPCODE::GTS);
    } else if (sv == "gtl") {
        return static_cast<opcode_type>(OPCODE::GTL);
    } else if (sv == "cast") {
        return static_cast<opcode_type>(OPCODE::CAST);
    } else if (sv == "arodp") {
        return static_cast<opcode_type>(OPCODE::ARODP);
    } else if (sv == "atxtp") {
        return static_cast<opcode_type>(OPCODE::ATXTP);
    } else if (sv == "sm") {
        return static_cast<opcode_type>(OPCODE::SM);
    } else if (sv == "lm") {
        return static_cast<opcode_type>(OPCODE::LM);
    } else if (sv == "ama") {
        return static_cast<opcode_type>(OPCODE::AA);
    } else if (sv == "amd") {
        return static_cast<opcode_type>(OPCODE::AD);
    } else if (sv == "ptr") {
        return static_cast<opcode_type>(OPCODE::PTR);
    } else if (sv == "earithmeticwidth") {
        return static_cast<opcode_type>(OPCODE::EARITHMETICWIDTH);
    } else {
        throw std::invalid_argument{ "viua::arch::ops::parse_opcode: "
                                     + std::string{ raw } };
    }
}

auto is_format(
    FORMAT const fmt,
    opcode_type const op) -> bool
{
    return static_cast<FORMAT>(op & OPCODE_FMT_MASK) == fmt;
}
auto is_format(
    FORMAT const fmt,
    OPCODE const op) -> bool
{
    return is_format(fmt, static_cast<opcode_type>(op));
}
}  // namespace viua::arch::ops
