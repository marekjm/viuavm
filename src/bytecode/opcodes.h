#ifndef WUDOO_BYTECODE_OPCODES_H
#define WUDOO_BYTECODE_OPCODES_H

#pragma once

#include "bytetypedef.h"

enum OPCODE : byte {
    NOP = 0,    // do nothing

    // integer instructions
    ISTORE,
    IADD,
    ISUB,
    IMUL,
    IDIV,
    IINC,
    IDEC,
    ILT,
    ILTE,
    IGT,
    IGTE,
    IEQ,

    // float instructions
    FSTORE,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FLT,
    FLTE,
    FGT,
    FGTE,
    FEQ,

    // integer instructions
    BSTORE,
    BADD,
    BSUB,
    BINC,
    BDEC,
    BLT,
    BLTE,
    BGT,
    BGTE,
    BEQ,

    // numeric conversion instructions
    ITOF,   // convert integer to float
    FTOI,   // convert float to integer

    // string instructions
    STRSTORE,
    STRADD,
    STREQ,

    // booleans
    BOOL,   // store Boolean false object in given register (empty) or
            // convert an object to Boolean value
    NOT,
    AND,
    OR,

    // register manipulation
    MOVE,   // move an object from one register to another
    COPY,   // copy an object from one register to another
    REF,    // create a refenrence to a value store in one register in another register
    SWAP,   // rotate two objects between their registers
    DELETE, // delete object from register
    ISNULL, // checks if register is null (empty) stores this information as a boolean in
            // another register
    RECONS, // REgister CONtext Switch - switches register context
    TMPRI,  // temporary register in - move object from current
            // register set into the temporary register
    TMPRO,  // temporary register out - move object out of the temporary
            // to current register set

    // printing
    PRINT,
    ECHO,

    // Opcodes related to functions.
    FRAME,  // create new frame (required before param and paref) for future function call
    PARAM,  // copy object from a register to parameter register (pass-by-value),
    PAREF,  // create a reference to an object in a parameter register (pass-by-reference),
    CALL,   // call given function with parameters set in parameter register,
    ARG,    // move an object from argument register to a normal register (inside a function call),

    JUMP,
    BRANCH,

    RET,
    END,

    PASS,   // do nothing
    HALT,
};

#endif
