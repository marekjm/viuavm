#ifndef WUDOO_BYTECODE_MAPS_H
#define WUDOO_BYTECODE_MAPS_H

#pragma once

#include <map>
#include <string>
#include "opcodes.h"


const std::map<std::string, unsigned> OP_SIZES = {
    { "nop",    sizeof(byte) },

    { "istore", sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "iadd",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "isub",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "imul",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "idiv",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "iinc",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "idec",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "ilt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "ilte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "igt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "igte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "ieq",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "bstore", sizeof(byte) + 2*sizeof(bool) + sizeof(int) + sizeof(byte) },
    { "badd",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "bsub",   sizeof(byte) + 2*sizeof(bool) + 2*sizeof(int) },
    { "binc",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "bdec",   sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "blt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "blte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "beq",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

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

    { "print",  sizeof(byte) + sizeof(bool) + sizeof(int) },
    { "echo",   sizeof(byte) + sizeof(bool) + sizeof(int) },

    { "param",  sizeof(byte) },
    { "paref",  sizeof(byte) },
    { "call",   sizeof(byte) },
    { "argmv",  sizeof(byte) },
    { "argc",   sizeof(byte) },

    { "jump",   sizeof(byte) + sizeof(int) },
    { "branch", sizeof(byte) + sizeof(bool) + 3*sizeof(int) },

    { "ret",    sizeof(byte) },
    { "end",    sizeof(byte) },

    { "pass",   sizeof(byte) },
    { "halt",   sizeof(byte) },
};


const std::map<enum OPCODE, std::string> OP_NAMES = {
    { NOP,	    "nop" },

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

    { BOOL,	    "bool" },
    { NOT,	    "not" },
    { AND,	    "and" },
    { OR,	    "or" },

    { MOVE,	    "move" },
    { COPY,	    "copy" },
    { REF,	    "ref" },
    { SWAP,	    "swap" },
    { DELETE,	"delete" },
    { ISNULL,	"isnull" },

    { PRINT,	"print" },
    { ECHO,	    "echo" },

    { PARAM,	"param" },
    { PAREF,	"paref" },
    { CALL,	    "call" },
    { ARGMV,	"argmv" },
    { ARGC,	    "argc" },

    { JUMP,	    "jump" },
    { BRANCH,	"branch" },

    { RET,	    "ret" },
    { END,	    "end" },

    { PASS,	    "pass" },
    { HALT,	    "halt" },
};


#endif
