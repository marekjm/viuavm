#ifndef TATANKA_BYTECODES_H
#define TATANKA_BYTECODES_H

#pragma once

enum Bytecode : unsigned int {
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

    NOT,
    AND,
    OR,

    PRINT,
    ECHO,

    JUMP,
    BRANCH,

    RET,
    END,

    PASS,
    HALT,
};

#endif
