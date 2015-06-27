#ifndef VIUA_BYTECODE_MAPS_H
#define VIUA_BYTECODE_MAPS_H

#pragma once

#include <map>
#include <vector>
#include <string>
#include "opcodes.h"


const std::map<std::string, unsigned> OP_SIZES = {
    { "nop",    sizeof(byte) },

    { "izero",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "istore", sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "iadd",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "isub",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "imul",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "idiv",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "iinc",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "idec",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "ilt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "ilte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "igt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "igte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "ieq",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "fstore", sizeof(byte) + sizeof(bool) + sizeof(int) + sizeof(float) },
    { "fadd",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "fsub",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "fmul",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "fdiv",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "flt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "flte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "fgt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "fgte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "feq",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "bstore", sizeof(byte) + 2*sizeof(bool) + sizeof(int) + sizeof(byte) },
    { "badd",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bsub",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "binc",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "bdec",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "blt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "blte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "beq",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "itof",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "ftoi",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "stoi",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "stof",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },

    // strstore <register> "string"
    { "strstore",sizeof(byte) + sizeof(bool) + sizeof(int) },
    // streq <str-a> <str-b> <dest>
    { "streq",  sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "vec",    sizeof(byte) + sizeof(bool) + sizeof(int) },        // vec      <register>
    { "vinsert",sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },    // vinsert  <vector> <src register> <dest index>?
    { "vpush",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },    // vpush    <vector> <src register>
    { "vpop",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },    // vpop     <vector> <result register>? <position>?
    { "vat",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },    // vat      <vector> <result register>  <position>?
    { "vlen",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },    // vlen     <vector> <result register>

    { "bool",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "not",    sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "and",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "or",     sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "move",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "copy",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "ref",    sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "swap",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "free",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "empty",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "isnull", sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "ress",   sizeof(byte) + sizeof(int) },
    { "tmpri",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "tmpro",  sizeof(byte) + sizeof(bool) + sizeof(int) },

    { "print",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "echo",   sizeof(byte) + sizeof(bool) + sizeof(int) },

    { "clbind", sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "closure",sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "clframe",sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "clcall", sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },

    { "function",sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "fcall",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },

    { "frame",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "param",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "paref",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "call",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "arg",    sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },

    { "jump",   sizeof(byte) + sizeof(int) },
    { "branch", sizeof(byte) + sizeof(bool) + 3*sizeof(int) },

    { "throw",  sizeof(byte) + sizeof(int) + sizeof(bool) },
    // variable length
    { "catch",  sizeof(byte) }, // catch "<type>" <block>
    { "pull",   sizeof(byte) + sizeof(bool) + sizeof(int) }, // pull <register>
    // variable length
    { "tryframe", sizeof(byte) },
    { "try",    sizeof(byte) }, // try <block>
    { "leave",  sizeof(byte) },

    { "eximport", sizeof(byte) },
    { "excall", sizeof(byte) + sizeof(bool) + sizeof(int) },

    { "end",    sizeof(byte) },
    { "halt",   sizeof(byte) },
};


const std::map<enum OPCODE, std::string> OP_NAMES = {
    { NOP,	    "nop" },

    { IZERO,    "izero" },
    { ISTORE,   "istore" },
    { IADD,     "iadd" },
    { ISUB,     "isub" },
    { IMUL,     "imul" },
    { IDIV,     "idiv" },
    { IINC,     "iinc" },
    { IDEC,     "idec" },
    { ILT,      "ilt" },
    { ILTE,     "ilte" },
    { IGT,      "igt" },
    { IGTE,     "igte" },
    { IEQ,      "ieq" },

    { FSTORE,   "fstore" },
    { FADD,     "fadd" },
    { FSUB,     "fsub" },
    { FMUL,     "fmul" },
    { FDIV,     "fdiv" },
    { FLT,      "flt" },
    { FLTE,     "flte" },
    { FGT,      "fgt" },
    { FGTE,     "fgte" },
    { FEQ,      "feq" },

    { BSTORE,   "bstore" },
    { BADD,     "badd" },
    { BSUB,     "bsub" },
    { BINC,     "binc" },
    { BDEC,     "bdec" },
    { BLT,      "blt" },
    { BLTE,     "blte" },
    { BGT,      "bgt" },
    { BGTE,     "bgte" },
    { BEQ,      "beq" },

    { ITOF,     "itof" },
    { FTOI,     "ftoi" },
    { STOI,     "stoi" },
    { STOF,     "stof" },

    { STRSTORE, "strstore" },
    { STREQ,    "streq" },

    { VEC,      "vec" },
    { VINSERT,  "vinsert" },
    { VPUSH,    "vpush" },
    { VPOP,     "vpop" },
    { VAT,      "vat" },
    { VLEN,     "vlen" },

    { BOOL,	    "bool" },
    { NOT,	    "not" },
    { AND,	    "and" },
    { OR,	    "or" },

    { MOVE,     "move" },
    { COPY,     "copy" },
    { REF,      "ref" },
    { SWAP,     "swap" },
    { FREE,     "free" },
    { EMPTY,    "empty" },
    { ISNULL,   "isnull" },
    { RESS,     "ress", },
    { TMPRI,    "tmpri", },
    { TMPRO,    "tmpro", },

    { PRINT,    "print" },
    { ECHO,     "echo" },

    { CLBIND,   "clbind" },
    { CLOSURE,  "closure" },
    { CLFRAME,  "clframe" },
    { CLCALL,   "clcall" },

    { FUNCTION, "function" },
    { FCALL,    "fcall" },

    { FRAME,    "frame" },
    { PARAM,    "param" },
    { PAREF,    "paref" },
    { CALL,     "call" },
    { ARG,      "arg" },

    { JUMP,     "jump" },
    { BRANCH,   "branch" },

    { THROW,    "throw" },
    { CATCH,    "catch" },
    { PULL,     "pull" },
    { TRYFRAME, "tryframe" },
    { TRY,      "try" },
    { LEAVE,    "leave" },

    { EXIMPORT, "eximport" },
    { EXCALL,   "excall" },

    { END,      "end" },
    { HALT,     "halt" },
};


const std::vector<enum OPCODE> OP_VARIABLE_LENGTH = {
    STRSTORE,
    CLOSURE,
    FUNCTION,
    CALL,
    CATCH,
    TRY,
    EXIMPORT,
    EXCALL,
};


#endif
