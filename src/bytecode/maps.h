#ifndef WUDOO_BYTECODE_MAPS_H
#define WUDOO_BYTECODE_MAPS_H

#pragma once

#include <map>
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

    { "bool",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "not",    sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "and",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "or",     sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "move",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "copy",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "ref",    sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "swap",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "delete", sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "isnull", sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "ress",   sizeof(byte) + sizeof(int) },
    { "tmpri",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "tmpro",  sizeof(byte) + sizeof(bool) + sizeof(int) },

    { "print",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "echo",   sizeof(byte) + sizeof(bool) + sizeof(int) },

    { "frame",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "param",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "paref",  sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    // this is because function call is followed by an instruction index, and
    // a register operand: one int is for index, second and bool for register
    { "call",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "arg",    sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },

    { "jump",   sizeof(byte) + sizeof(int) },
    { "branch", sizeof(byte) + sizeof(bool) + 3*sizeof(int) },

    { "end",    sizeof(byte) },

    { "pass",   sizeof(byte) },
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

    { BOOL,	    "bool" },
    { NOT,	    "not" },
    { AND,	    "and" },
    { OR,	    "or" },

    { MOVE,     "move" },
    { COPY,     "copy" },
    { REF,      "ref" },
    { SWAP,     "swap" },
    { DELETE,   "delete" },
    { ISNULL,   "isnull" },
    { RESS,     "ress", },
    { TMPRI,    "tmpri", },
    { TMPRO,    "tmpro", },

    { PRINT,    "print" },
    { ECHO,     "echo" },

    { FRAME,    "frame" },
    { PARAM,    "param" },
    { PAREF,    "paref" },
    { CALL,     "call" },
    { ARG,      "arg" },

    { JUMP,     "jump" },
    { BRANCH,   "branch" },

    { END,      "end" },

    { PASS,     "pass" },
    { HALT,     "halt" },
};


#endif
