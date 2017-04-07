/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <vector>
#include <set>
#include <string>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>


const std::set<std::string> OP_MNEMONICS = {
    "nop",

    "izero",
    "istore",
    "iinc",
    "idec",

    "fstore",

    "itof",
    "ftoi",
    "stoi",
    "stof",

    "add",
    "sub",
    "mul",
    "div",
    "lt",
    "lte",
    "gt",
    "gte",
    "eq",

    "strstore",
    "streq",

    "text",
    "texteq",
    "textat",
    "textsub",
    "textlength",
    "textcommonprefix",
    "textcommonsuffix",
    "textview",
    "textconcat",

    "vec",
    "vinsert",
    "vpush",
    "vpop",
    "vat",
    "vlen",

    "bool",
    "not",
    "and",
    "or",

    "move",
    "copy",
    "ptr",
    "swap",
    "delete",
    "isnull",
    "ress",

    "print",
    "echo",

    "capture",
    "capturecopy",
    "capturemove",
    "closure",

    "function",

    "frame",
    "param",
    "pamv",
    "call",
    "tailcall",
    "arg",
    "argc",
    "process",
    "self",
    "join",
    "send",
    "receive",

    "watchdog",

    "jump",
    "if",

    "throw",
    "catch",
    "draw",
    "try",
    "enter",
    "leave",

    "import",
    "link",

    "class",
    "derive",
    "attach",
    "register",

    "new",
    "msg",
    "insert",
    "remove",

    "return",
    "halt",
};


const std::map<enum OPCODE, std::string> OP_NAMES = {
    { NOP,	        "nop" },

    { IZERO,        "izero" },
    { ISTORE,       "istore" },
    { IINC,         "iinc" },
    { IDEC,         "idec" },

    { FSTORE,       "fstore" },

    { ITOF,         "itof" },
    { FTOI,         "ftoi" },
    { STOI,         "stoi" },
    { STOF,         "stof" },

    { ADD,          "add" },
    { SUB,          "sub" },
    { MUL,          "mul" },
    { DIV,          "div" },
    { LT,           "lt" },
    { LTE,          "lte" },
    { GT,           "gt" },
    { GTE,          "gte" },
    { EQ,           "eq" },

    { STRSTORE,     "strstore" },
    { STREQ,        "streq" },

    { TEXT,         "text" },
    { TEXTEQ,       "texteq" },
    { TEXTAT,       "textat" },
    { TEXTSUB,      "textsub" },
    { TEXTLENGTH,   "textlength" },
    { TEXTCOMMONPREFIX, "textcommonprefix" },
    { TEXTCOMMONSUFFIX, "textcommonsuffix" },
    { TEXTVIEW,     "textview" },
    { TEXTCONCAT,   "textconcat" },

    { VEC,          "vec" },
    { VINSERT,      "vinsert" },
    { VPUSH,        "vpush" },
    { VPOP,         "vpop" },
    { VAT,          "vat" },
    { VLEN,         "vlen" },

    { BOOL,	        "bool" },
    { NOT,	        "not" },
    { AND,	        "and" },
    { OR,	        "or" },

    { MOVE,         "move" },
    { COPY,         "copy" },
    { PTR,          "ptr" },
    { SWAP,         "swap" },
    { DELETE,       "delete" },
    { ISNULL,       "isnull" },
    { RESS,         "ress", },

    { PRINT,        "print" },
    { ECHO,         "echo" },

    { CAPTURE,      "capture" },
    { CAPTURECOPY,  "capturecopy" },
    { CAPTUREMOVE,  "capturemove" },
    { CLOSURE,      "closure" },

    { FUNCTION,     "function" },

    { FRAME,        "frame" },
    { PARAM,        "param" },
    { PAMV,         "pamv" },
    { CALL,         "call" },
    { TAILCALL,     "tailcall" },
    { ARG,          "arg" },
    { ARGC,         "argc" },
    { PROCESS,      "process" },
    { SELF,         "self" },
    { JOIN,         "join" },
    { SEND,         "send" },
    { RECEIVE,      "receive" },
    { WATCHDOG,     "watchdog" },

    { JUMP,         "jump" },
    { IF,           "if" },

    { THROW,        "throw" },
    { CATCH,        "catch" },
    { DRAW,         "draw" },
    { TRY,          "try" },
    { ENTER,        "enter" },
    { LEAVE,        "leave" },

    { IMPORT,       "import" },
    { LINK,         "link" },

    { CLASS,        "class" },
    { DERIVE,       "derive" },
    { ATTACH,       "attach" },
    { REGISTER,     "register" },

    { NEW,          "new" },
    { MSG,          "msg" },
    { INSERT,       "insert" },
    { REMOVE,       "remove" },

    { RETURN,       "return" },
    { HALT,         "halt" },
};


#endif
