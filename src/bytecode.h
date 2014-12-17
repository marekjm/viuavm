#ifndef TATANKA_BYTECODES_H
#define TATANKA_BYTECODES_H

#pragma once

enum Bytecode {
    ISTORE,     // ISTORE <register> <number>
    IADD,       // IADD <register-a> <register-b> <register-result> (a + b)
    ISUB,       // ISUB <register-a> <register-b> <register-result> (a - b)
    IMUL,       // IMUL <register-a> <register-b> <register-result> (a * b)
    IDIV,       // IDIV <register-a> <register-b> <register-result> (a / b)

    IPRINT,     // IPRINT <register-index>

    ILT,        // ILT  <reg-a> <reg-b> <reg-result> (a < b)
    ILTE,       // ILTE <reg-a> <reg-b> <reg-result> (a <= b)
    IGT,        // IGT  <reg-a> <reg-b> <reg-result> (a > b)
    IGTE,       // IGTE <reg-a> <reg-b> <reg-result> (a >= b)
    IEQ,        // IEQ  <reg-a> <reg-b> <reg-result> (a == b)

    BRANCH,     // BRANCH <bytecode-index>
    BRANCHIF,   // BRANCHIF <reg-bool> <bytecode-index-if-true> <bytecode-index-if-false>

    CALL,       // CALL <bytecode-index> <register-for-return-value> <number-of-parameters> <first-param-reg-index>
                // "CALL 0x4f 17 2 29"
                // is a CALL to function starting at bytecode 0x4f, storing its return value in register 17.
                // 0x4f is a function of two parameters, with first parameter placed in register 29.

    RET,        // RET <reg-index>
    END,        // END  (end, e.g. function call)

    HALT,       // HALT
};

#endif
