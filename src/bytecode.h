#ifndef TATANKA_BYTECODES_H
#define TATANKA_BYTECODES_H

#pragma once

enum Bytecode {
    ISTORE,     // ISTORE <register> <number>
    IADD,       // IADD <register-a> <register-b> <register-result> (a + b)
    ISUB,       // ISUB <register-a> <register-b> <register-result> (a - b)
    IMUL,       // IMUL <register-a> <register-b> <register-result> (a * b)
    IDIV,       // IDIV <register-a> <register-b> <register-result> (a / b)
    IINC,       // IINC <register-index>    (increment integer at given register)
    IDEC,       // IDEC <register-index>    (decrement integer at given register)

    ILT,        // ILT  <reg-a> <reg-b> <reg-result> (a < b)
    ILTE,       // ILTE <reg-a> <reg-b> <reg-result> (a <= b)
    IGT,        // IGT  <reg-a> <reg-b> <reg-result> (a > b)
    IGTE,       // IGTE <reg-a> <reg-b> <reg-result> (a >= b)
    IEQ,        // IEQ  <reg-a> <reg-b> <reg-result> (a == b)

    STRSTORE,
    STRADD,
    STREQ,

    PRINT,      // PRINT <register-index>

    BRANCH,     // BRANCH <bytecode-index>
    BRANCHIF,   // BRANCHIF <reg-bool> <bytecode-index-if-true> <bytecode-index-if-false>

    RET,        // RET <reg-index>
    END,        // END  (end, e.g. function call)

    PASS,       // "do nothing" instruction
    HALT,       // HALT
};

#endif
