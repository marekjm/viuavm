#include <map>
#include <string>
#include "opcodes.h"


typedef char byte;


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
    { "binc",   sizeof(byte) },
    { "bdec",   sizeof(byte) },
    { "blt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "blte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgt",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "bgte",   sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },
    { "beq",    sizeof(byte) + 3*sizeof(bool) + 3*sizeof(int) },

    { "bool",   sizeof(byte) },
    { "not",    sizeof(byte) },
    { "and",    sizeof(byte) },
    { "or",     sizeof(byte) },

    { "move",   sizeof(byte) },
    { "copy",   sizeof(byte) },
    { "ref",    sizeof(byte) },
    { "swap",   sizeof(byte) },
    { "delete", sizeof(byte) },
    { "isnull", sizeof(byte) },

    { "print",  sizeof(byte) },
    { "echo",   sizeof(byte) },

    { "param",  sizeof(byte) },
    { "paref",  sizeof(byte) },
    { "call",   sizeof(byte) },
    { "argmv",  sizeof(byte) },
    { "argc",   sizeof(byte) },

    { "jump",   sizeof(byte) },
    { "branch", sizeof(byte) },

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
