/*
 *  Copyright (C) 2015-2017, 2019 Marek Marecki
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

#ifndef VIUA_BYTECODE_MAPS_H
#define VIUA_BYTECODE_MAPS_H

#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>


std::map<enum OPCODE, std::string> const OP_NAMES = {
    {NOP, "nop"},

    {IZERO, "izero"},
    {INTEGER, "integer"},
    {IINC, "iinc"},
    {IDEC, "idec"},

    {FLOAT, "float"},

    {ITOF, "itof"},
    {FTOI, "ftoi"},
    {STOI, "stoi"},
    {STOF, "stof"},

    {ADD, "add"},
    {SUB, "sub"},
    {MUL, "mul"},
    {DIV, "div"},
    {LT, "lt"},
    {LTE, "lte"},
    {GT, "gt"},
    {GTE, "gte"},
    {EQ, "eq"},

    {STRING, "string"},
    {STREQ, "streq"},

    {TEXT, "text"},
    {TEXTEQ, "texteq"},
    {TEXTAT, "textat"},
    {TEXTSUB, "textsub"},
    {TEXTLENGTH, "textlength"},
    {TEXTCOMMONPREFIX, "textcommonprefix"},
    {TEXTCOMMONSUFFIX, "textcommonsuffix"},
    {TEXTCONCAT, "textconcat"},

    {VECTOR, "vector"},
    {VINSERT, "vinsert"},
    {VPUSH, "vpush"},
    {VPOP, "vpop"},
    {VAT, "vat"},
    {VLEN, "vlen"},

    {BOOL, "bool"},
    {NOT, "not"},
    {AND, "and"},
    {OR, "or"},

    {BITS, "bits"},
    {BITAND, "bitand"},
    {BITOR, "bitor"},
    {BITNOT, "bitnot"},
    {BITXOR, "bitxor"},
    {BITSWIDTH, "bitswidth"},
    {BITAT, "bitat"},
    {BITSET, "bitset"},
    {SHL, "shl"},
    {SHR, "shr"},
    {ASHL, "ashl"},
    {ASHR, "ashr"},
    {ROL, "rol"},
    {ROR, "ror"},

    {BITSEQ, "bitseq"},
    {BITSLT, "bitslt"},
    {BITSLTE, "bitslte"},
    {BITSGT, "bitsgt"},
    {BITSGTE, "bitsgte"},

    {BITAEQ, "bitaeq"},
    {BITALT, "bitalt"},
    {BITALTE, "bitalte"},
    {BITAGT, "bitagt"},
    {BITAGTE, "bitagte"},

    {WRAPINCREMENT, "wrapincrement"},
    {WRAPDECREMENT, "wrapdecrement"},
    {WRAPADD, "wrapadd"},
    {WRAPSUB, "wrapsub"},
    {WRAPMUL, "wrapmul"},
    {WRAPDIV, "wrapdiv"},

    {CHECKEDSINCREMENT, "checkedsincrement"},
    {CHECKEDSDECREMENT, "checkedsdecrement"},
    {CHECKEDSADD, "checkedsadd"},
    {CHECKEDSSUB, "checkedssub"},
    {CHECKEDSMUL, "checkedsmul"},
    {CHECKEDSDIV, "checkedsdiv"},

    {CHECKEDUINCREMENT, "checkeduincrement"},
    {CHECKEDUDECREMENT, "checkedudecrement"},
    {CHECKEDUADD, "checkeduadd"},
    {CHECKEDUSUB, "checkedusub"},
    {CHECKEDUMUL, "checkedumul"},
    {CHECKEDUDIV, "checkedudiv"},

    {SATURATINGSINCREMENT, "saturatingsincrement"},
    {SATURATINGSDECREMENT, "saturatingsdecrement"},
    {SATURATINGSADD, "saturatingsadd"},
    {SATURATINGSSUB, "saturatingssub"},
    {SATURATINGSMUL, "saturatingsmul"},
    {SATURATINGSDIV, "saturatingsdiv"},

    {SATURATINGUINCREMENT, "saturatinguincrement"},
    {SATURATINGUDECREMENT, "saturatingudecrement"},
    {SATURATINGUADD, "saturatinguadd"},
    {SATURATINGUSUB, "saturatingusub"},
    {SATURATINGUMUL, "saturatingumul"},
    {SATURATINGUDIV, "saturatingudiv"},

    {MOVE, "move"},
    {COPY, "copy"},
    {PTR, "ptr"},
    {PTRLIVE, "ptrlive"},
    {SWAP, "swap"},
    {DELETE, "delete"},
    {ISNULL, "isnull"},

    {PRINT, "print"},
    {ECHO, "echo"},

    {CAPTURE, "capture"},
    {CAPTURECOPY, "capturecopy"},
    {CAPTUREMOVE, "capturemove"},
    {CLOSURE, "closure"},

    {FUNCTION, "function"},

    {FRAME, "frame"},
    {PARAM, "param"},
    {PAMV, "pamv"},
    {CALL, "call"},
    {TAILCALL, "tailcall"},
    {DEFER, "defer"},
    {ARG, "arg"},
    {ALLOCATE_REGISTERS, "allocate_registers"},
    {PROCESS, "process"},
    {SELF, "self"},
    {JOIN, "join"},
    {SEND, "send"},
    {RECEIVE, "receive"},
    {WATCHDOG, "watchdog"},

    {JUMP, "jump"},
    {IF, "if"},

    {THROW, "throw"},
    {CATCH, "catch"},
    {DRAW, "draw"},
    {TRY, "try"},
    {ENTER, "enter"},
    {LEAVE, "leave"},

    {IMPORT, "import"},

    {ATOM, "atom"},
    {ATOMEQ, "atomeq"},

    {STRUCT, "struct"},
    {STRUCTINSERT, "structinsert"},
    {STRUCTREMOVE, "structremove"},
    {STRUCTAT, "structat"},
    {STRUCTKEYS, "structkeys"},

    {RETURN, "return"},
    {HALT, "halt"},
};


#endif
