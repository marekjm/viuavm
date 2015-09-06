#ifndef VIUA_BYTECODE_OPCODES_H
#define VIUA_BYTECODE_OPCODES_H

#pragma once

#include <viua/bytecode/bytetypedef.h>

enum OPCODE : byte {
    NOP = 0,    // do nothing

    // integer instructions
    IZERO,
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

    // byte instructions
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
    STOI,   // convert string to integer
    STOF,   // convert string to float

    // string instructions
    STRSTORE,
    STREQ,

    VEC,
    VINSERT,
    VPUSH,
    VPOP,
    VAT,
    VLEN,

    // booleans
    BOOL,   // store Boolean false object in given register (empty) or
            // convert an object to Boolean value
    NOT,
    AND,
    OR,

    // register manipulation
    MOVE,   // move an object from one register to another
    COPY,   // copy an object from one register to another
    REF,    // create a refenrence to an object
    SWAP,   // swap two objects between registers
    FREE,   // delete an object from a register, freeing the memory
    EMPTY,  // empty the register and set its flags to 0
    ISNULL, // checks if register is null (empty)
    RESS,   // REgister Set Switch - switches register set
    TMPRI,  // temporary register in - move object from current
            // register set into the temporary register
    TMPRO,  // temporary register out - move object out of the temporary
            // to current register set

    PRINT,
    ECHO,

    CLBIND,
    CLOSURE,

    FUNCTION,
    FCALL,

    // Opcodes related to functions.
    FRAME,  // create new frame (required before param and paref) for future function call
    PARAM,  // copy object from a register to parameter register (pass-by-value),
    PAREF,  // create a reference to an object in a parameter register (pass-by-reference),
    CALL,   // call given function with parameters set in parameter register,
    ARG,    // move an object from argument register to a normal register (inside a function call),
    ARGC,   // store number of supplied parameters in a register

    JUMP,
    BRANCH,

    THROW,      // throw an object
    CATCH,      // register a catcher block for given type
    PULL,       // pull caught object into a register (it becomes local object for current frame)
    TRYFRAME,   // create a frame for try block
    TRY,        // try executing a block, if an exception is thrown and no catcher claims it, it is propagated up
                // TRY instructions do not require any CATCH to precede them
    LEAVE,      // leave a block and resume execution after block-entering instruction

    IMPORT,     // dynamically link foreign library
    LINK,       // dynamically link native library

    CLASS,      // create a prototype for new class
    PROTOTYPE,  // create a prototype from existing class
    DERIVE,     // derive a prototype from an existing class
    ATTACH,     // attach a method to the prototype
    REGISTER,   // register a prototype in VM's typesystem

    NEW,        // construct new instance of a class in a register
    MSG,        // send a message to an object (used for dynamic dispatch, for static use plan "CALL")

    END,
    HALT,
};

#endif
