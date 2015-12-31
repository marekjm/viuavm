#ifndef VIUA_BYTECODE_MAPS_H
#define VIUA_BYTECODE_MAPS_H

#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>


const std::map<std::string, unsigned> OP_SIZES = {
    { "nop",    sizeof(byte) },

    { "izero",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "istore", sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "iadd",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "isub",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "imul",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "idiv",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "iinc",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "idec",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "ilt",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "ilte",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "igt",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "igte",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "ieq",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },

    { "fstore", sizeof(byte) + sizeof(OperandType) + sizeof(int) + sizeof(float) },
    { "fadd",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "fsub",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "fmul",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "fdiv",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "flt",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "flte",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "fgt",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "fgte",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "feq",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },

    { "bstore", sizeof(byte) + 2*sizeof(OperandType) + sizeof(int) + sizeof(byte) },
    { "badd",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "bsub",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "binc",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "bdec",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "blt",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "blte",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "bgt",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "bgte",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "beq",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },

    { "itof",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "ftoi",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "stoi",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "stof",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },

    { "strstore",sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "streq",  sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },

    { "vec",    sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "vinsert",sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "vpush",  sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "vpop",   sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "vat",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "vlen",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },

    { "bool",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "not",    sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "and",    sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },
    { "or",     sizeof(byte) + 3*sizeof(OperandType) + 3*sizeof(int) },

    { "move",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "copy",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "ref",    sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "swap",   sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "delete", sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "empty",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "isnull", sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "ress",   sizeof(byte) + sizeof(int) },
    { "tmpri",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "tmpro",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },

    { "print",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "echo",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },

    { "clbind", sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "closure",sizeof(byte) + sizeof(OperandType) + sizeof(int) },

    { "function",sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "fcall",  sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },

    { "frame",  sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "param",  sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "paref",  sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "call",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "arg",    sizeof(byte) + 2*sizeof(OperandType) + 2*sizeof(int) },
    { "argc",   sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "thread", sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "thjoin", sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "threceive", sizeof(byte) + sizeof(OperandType) + sizeof(int) },

    { "jump",   sizeof(byte) + sizeof(uint64_t) },
    { "branch", sizeof(byte) + sizeof(OperandType)+sizeof(int) + 2*sizeof(uint64_t) },

    { "throw",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },
    { "catch",  sizeof(byte) }, // catch "<type>" <block>
    { "pull",   sizeof(byte) + sizeof(OperandType) + sizeof(int) }, // pull <register>
    { "try",    sizeof(byte) },
    { "enter",  sizeof(byte) },
    { "leave",  sizeof(byte) },

    { "import", sizeof(byte) },
    { "link",   sizeof(byte) },

    { "class",  sizeof(byte) + sizeof(OperandType) + sizeof(int) },    // class 1 Foo
    { "prototype",sizeof(byte) + sizeof(OperandType) + sizeof(int) },  // prototype 1 Foo
    { "derive", sizeof(byte) + sizeof(OperandType) + sizeof(int) },    // derive 1 Bar
    { "attach", sizeof(byte) + sizeof(OperandType) + sizeof(int) },    // attach 1 function method
    { "register",sizeof(byte) + sizeof(OperandType) + sizeof(int) },   // register 1

    { "new",    sizeof(byte) + sizeof(OperandType) + sizeof(int) },    // new <target> Foo
    { "msg",    sizeof(byte) + sizeof(OperandType) + sizeof(int) },    // msg <return> method

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
    { DELETE,   "delete" },
    { EMPTY,    "empty" },
    { ISNULL,   "isnull" },
    { RESS,     "ress", },
    { TMPRI,    "tmpri", },
    { TMPRO,    "tmpro", },

    { PRINT,    "print" },
    { ECHO,     "echo" },

    { CLBIND,   "clbind" },
    { CLOSURE,  "closure" },

    { FUNCTION, "function" },
    { FCALL,    "fcall" },

    { FRAME,    "frame" },
    { PARAM,    "param" },
    { PAREF,    "paref" },
    { CALL,     "call" },
    { ARG,      "arg" },
    { ARGC,     "argc" },
    { THREAD,   "thread" },
    { THJOIN,   "thjoin" },
    { THRECEIVE,"threceive" },

    { JUMP,     "jump" },
    { BRANCH,   "branch" },

    { THROW,    "throw" },
    { CATCH,    "catch" },
    { PULL,     "pull" },
    { TRY,      "try" },
    { ENTER,    "enter" },
    { LEAVE,    "leave" },

    { IMPORT,   "import" },
    { LINK,     "link" },

    { CLASS,    "class" },
    { PROTOTYPE,"prototype" },
    { DERIVE,   "derive" },
    { ATTACH,   "attach" },
    { REGISTER, "register" },

    { NEW,      "new" },
    { MSG,      "msg" },

    { END,      "end" },
    { HALT,     "halt" },
};


const std::vector<enum OPCODE> OP_VARIABLE_LENGTH = {
    STRSTORE,
    CLOSURE,
    FUNCTION,
    CALL,
    THREAD,
    CATCH,
    ENTER,
    IMPORT,
    LINK,
    CLASS,
    PROTOTYPE,
    DERIVE,
    ATTACH,
    NEW,
    MSG,
};


#endif
