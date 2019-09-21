/*
 *  Copyright (C) 2015-2017, 2019 Marek Marecki
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

#ifndef VIUA_BYTECODE_OPCODES_H
#define VIUA_BYTECODE_OPCODES_H

#pragma once

#include <viua/bytecode/bytetypedef.h>

enum OPCODE : viua::internals::types::byte {
    NOP = 0,  // do nothing

    // integer instructions
    IZERO,
    INTEGER,
    IINC,
    IDEC,

    // float instructions
    FLOAT,

    // numeric conversion instructions
    ITOF,  // convert integer to float
    FTOI,  // convert float to integer
    STOI,  // convert string to integer
    STOF,  // convert string to float

    ADD,
    SUB,
    MUL,
    DIV,
    LT,
    LTE,
    GT,
    GTE,
    EQ,

    // string instructions
    STRING,
    STREQ,

    /*
     *  Store a text value in a register.
     *
     *  text {target-register} "<text>"
     */
    TEXT,

    /*
     *  Compare two text values for equality.
     *
     *  texteq {result-register} {lhs-register} {rhs-register}
     */
    TEXTEQ,

    /*
     *  Return copy of the character at a given index in text.
     *
     *  textat {result-register} {string-register} {index-register}
     */
    TEXTAT,

    /*
     *  Return a copy of a part of the given text between given indexes.
     *
     *  textsub {result-register} {string-register} {begin-index-register}
     * {end-index:register|void}
     */
    TEXTSUB,

    /*
     *  Return length of a given text value (in characters).
     *
     *  textlength {result-register} {string-register}
     */
    TEXTLENGTH,

    /*
     *  Return length of common prefix of two text values.
     *
     *  textcommonprefix {result-register} {lhs-string-register}
     * {rhs-string-register}
     */
    TEXTCOMMONPREFIX,

    /*
     *  Return length of common suffix of two text values.
     *
     *  textcommonsuffix {result-register} {lhs-string-register}
     * {rhs-string-register}
     */
    TEXTCOMMONSUFFIX,

    /*
     *  Concatenate two text values. Creates a copy of each text value.
     *
     *  textconcat {result-register} {lhs-string-register} {rhs-string-register}
     */
    TEXTCONCAT,

    VECTOR,
    VINSERT,
    VPUSH,
    VPOP,
    VAT,
    VLEN,

    /*
     * Any Viua VM value may be converted to a boolean value.
     */
    BOOL,  // store Boolean false object in given register (empty) or
           // convert an object to Boolean value
    NOT,   // logical negation of any value
    AND,   // logical and of any two values
    OR,    // logical or of any two values

    BITS_OF_INTEGER,
    INTEGER_OF_BITS,

    BITS,    // create a bits value from a literal, or a zero-filled value of a
             // specified width
    BITAND,  // binary and of two bits values
    BITOR,   // binary or of two bits values
    BITNOT,  // negation of a bits value
    BITXOR,  // binary xor of two bits values

    BITSWIDTH,  // fetch width of a bits value
    BITAT,      // fetch the bit at the requested offset
    BITSET,     // set the bit at the requested offset to a specified value

    SHL,   // logical shift left
    SHR,   // logical shift right
    ASHL,  // arithmetic shift left
    ASHR,  // arithmetic shift right
    ROL,   // rotate left
    ROR,   // rotate right


    /*
     * Bit string comparison operations.
     *
     * These instructions treat bit strings as strings.
     * Comparison starts with most significant bit, and
     * ends with the least significant bit.
     *
     * Longer string is *always* greater; '00' is greater than '0'.
     * This is because the strings are compared as if they were equal in width,
     * and the shorter one is padded on the left with a magic "very small value"
     * that is actually lesser than '0' bit.
     *
     * Some example comparisons evaluating to truth:
     *
     *      bitseq   0   0
     *      bitseq   1   1
     *
     *      bitslt   0   1
     *      bitslt   1  10
     *
     *      bitslte  0   0
     *      bitslte 10  11
     *
     *      bitsgt   00   0
     *      bitsgt    1   0
     *      bitsgt  000  11
     *
     *      bitsgte  00   0
     *      bitsgte 101 100
     *      bitsgte 011 010
     *
     * For meaningful comparison it is best that compared bit strings are
     * the same width.
     */
    BITSEQ,
    BITSLT,
    BITSLTE,
    BITSGT,
    BITSGTE,

    /*
     * Bit string arithmetic comparison operations.
     *
     * These instructions treat bit strings as signed values.
     * The operands do not have to be the same width to be correctly compared.
     */
    BITAEQ,
    BITALT,
    BITALTE,
    BITAGT,
    BITAGTE,

    /*
     * Fixed-width, wrap-aroud arithmetic operations on bit strings.
     *
     * These operations produce results that have the same width
     * as the left-hand side operand.
     * For example:
     *
     *      wrapmul %result %lhs %rhs
     *
     * produces 8 bit wide value in 'result' register if
     * register 'lhs' contains 8 bit wide value.
     *
     * Result values that are out of range of left-hand side
     * operand are "clipped", i.e. only N least significant bits
     * are preserved.
     * This produces the wrap-around effect; values out of range available
     * for given bit width are reduced by modulo 2 to n, where 'n' is the
     * bit width of the left-hand side operand.
     * For example, if the left-hand side operand is an 8 bit wide value, and
     * the result would be 12 bits wide, only the 8 least significat bits
     * will be put in the result register:
     *
     *      101111010010    (full result)
     *      ----11010010    (removing four most-significant bits)
     *
     *          11010010    (final result)
     *
     * Wrapping instructions are provided in only one variant that is suitable
     * for use with both signed and unsigned integers since their behaviour is
     * the same (wrap-around on overflow).
     */
    WRAPINCREMENT,
    WRAPDECREMENT,
    WRAPADD,
    WRAPSUB,
    WRAPMUL,
    WRAPDIV,

    /*
     * Fixed-width, checked arithmetic operations on bit strings.
     *
     * These operations produce results that have the same width
     * as the left-hand-dise operand.
     * For example:
     *
     *      checkedsmul %result %lhs %rhs
     *
     * produces 8 bit wide value in 'result' register if
     * register 'lhs' contains 8 bit wide value.
     *
     * If any checked operation produces a value that is out of range
     * for the bit width specified by the left-hand side operand, or
     * when the value produced is in range but causes an overflow (as
     * is the case with signed integers) an exception is thrown.
     *
     * Every operation is provided in signed and unsigned variant.
     * Mnemonics "checked?op" where "?" is "s" stand for "signed", and
     * where "?" is "u" stand for "unsigned".
     * For example, "checkedsmul" means "checked signed multiplication".
     */
    CHECKEDSINCREMENT,
    CHECKEDSDECREMENT,
    CHECKEDSADD,
    CHECKEDSSUB,
    CHECKEDSMUL,
    CHECKEDSDIV,

    CHECKEDUINCREMENT,
    CHECKEDUDECREMENT,
    CHECKEDUADD,
    CHECKEDUSUB,
    CHECKEDUMUL,
    CHECKEDUDIV,

    /*
     * Fixed-width, sarurating arithmetic operations on bit strings.
     *
     * These operations produce results that have the same width
     * as the left-hand-dise operand.
     * For example:
     *
     *      saturatingmul %result %lhs %rhs
     *
     * produces 8 bit wide value in 'result' register if
     * register 'lhs' contains 8 bit wide value.
     *
     * If any checked operation results in an overflow or underflow
     * for the bit width specified by the left-hand side operand the value
     * becomes "saturated".
     *
     * For example, using 8 bit wide numbers for simplicity, when 16 is
     * added to 255 the result is 255 as the value already is at the maximum;
     * it is already "saturated" and cannot hold any "more" digits.
     * If 16 is subtracted from 0 (assuming unsigned values) the result is zero
     * because the value is already at the minimum.
     */
    SATURATINGSINCREMENT,
    SATURATINGSDECREMENT,
    SATURATINGSADD,
    SATURATINGSSUB,
    SATURATINGSMUL,
    SATURATINGSDIV,

    SATURATINGUINCREMENT,
    SATURATINGUDECREMENT,
    SATURATINGUADD,
    SATURATINGUSUB,
    SATURATINGUMUL,
    SATURATINGUDIV,

    // register manipulation
    MOVE,     // move an object from one register to another
    COPY,     // copy an object from one register to another
    PTR,      // create a pointer to an object
    PTRLIVE,  // check if pointer is live
    SWAP,     // swap two objects between registers
    DELETE,   // delete an object from a register, freeing the memory
    ISNULL,   // checks if register is null (empty)

    PRINT,
    ECHO,

    CAPTURE,
    CAPTURECOPY,
    CAPTUREMOVE,
    CLOSURE,

    FUNCTION,

    // Opcodes related to functions.
    FRAME,  // create new frame (required before param and pamv) for future
            // function call
    PARAM,  // copy object from a register to parameter register
            // (pass-by-value),
    PAMV,   // move object from a register to parameter register (pass-by-move),
    CALL,   // call given function with parameters set in parameter register,
    TAILCALL,  // perform a tail call to a function
    DEFER,  // call a function just after the frame it was called in is popped
            // off the stack
    ARG,  // move an object from argument register to a normal register (inside
          // a function call),
    ALLOCATE_REGISTERS,  // allocates specified number of registers

    PROCESS,   // spawn a process (call a function and run it in a different
               // process)
    SELF,      // store a PID of the running process in a register
    PIDEQ,     // compare two PIDs for equality
    JOIN,      // join a process
    SEND,      // send a message to a process
    RECEIVE,   // receive passed message, block until one arrives
    WATCHDOG,  // spawn watchdog process

    JUMP,
    IF,

    THROW,  // throw an object
    CATCH,  // register a catcher block for given type
    DRAW,   // move caught object into a register (it becomes local object for
            // current frame) the "draw" name has been chosen because the
            // sequence of events:
            //
            //  1) an object is thrown
            //  2) that object is caught
            //  3) the catcher draws the object into a register
            //
            // nicely describes the situation these instructions model.

    TRY,    // create a frame for try block
    ENTER,  // enter a block, if an exception is thrown and no catcher claims
            // it, it is propagated up ENTER instructions do not require any
            // CATCH to precede them
    LEAVE,  // leave a block and resume execution after last enter instruction

    IMPORT,  // dynamically link code modules

    /*
     *  Create an atom.
     *  Atoms can be compared for equality, and encode symbols (function names,
     * type names, etc.).
     *
     *  atom {target-register} '{atom-value}'
     */
    ATOM,

    /*
     *  Compare atoms for equality.
     *
     *  atomeq {result-register} {lhs-register} {rhs-register}
     */
    ATOMEQ,

    /*
     *  Create a struct.
     *  Structs are anonymous key-value containers.
     *
     *  struct {target-register}
     */
    STRUCT,

    /*
     *  Insert a value into a struct at a given key.
     *  Moves the value into the struct.
     *
     *  structinsert {target-struct-register} {key-register} {value-register}
     */
    STRUCTINSERT,

    /*
     *  Remove a value at a given key from a struct, and return removed value.
     *  Throws an exception if the struct does not have requested key.
     *
     *  structremove {result-register} {source-struct-register} {key-register}
     */
    STRUCTREMOVE,

    /*
     *  Access a value at a given key from a struct, and return pointer to the
     *  value.
     *  Throws an exception if the struct does not have requested key.
     *
     *  structat {result-register} {source-struct-register} {key-register}
     */
    STRUCTAT,

    /*
     *  Get a vector with keys of a struct.
     *
     *  structkeys {result} {source-struct-register}
     */
    STRUCTKEYS,

    IO_READ,
    IO_WRITE,
    IO_CLOSE,
    IO_WAIT,
    IO_CANCEL,

    RETURN,
    HALT,
};

#endif
