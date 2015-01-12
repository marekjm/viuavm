#ifndef WUDOO_BYTECODE_OPCODES_H
#define WUDOO_BYTECODE_OPCODES_H

#pragma once

typedef unsigned char byte;

enum OPCODE : byte {
    NOP = 0,    // do nothing

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

    PRINT,
    ECHO,

    // Opcodes related to functions.
    PARAM,  // copy object from a register to parameter register (pass-by-value),
    PAREF,  // create a reference to an object in a parameter register (pass-by-reference),
    CALL,   // call given function with parameters set in parameter register,
    ARGMV,  // move an object from argument register to a normal register (in a function call),
    ARGC,   // store number of passed arguments in a given register (an Unsigned Integer type),

    JUMP,
    BRANCH,

    RET,
    END,

    PASS,   // do nothing
    HALT,
};

#endif
