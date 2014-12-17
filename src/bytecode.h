#ifndef TATANKA_BYTECODES_H
#define TATANKA_BYTECODES_H

#pragma once

enum Bytecode {
    ISTORE,
    IADD,

    PRINT_I,

    BRANCH,
    HALT,
};


class Instruction {
    public:
        Bytecode which;     // type of instruction
        void** registers;   // addresses of register operands
        int* locals;
        int operands;       // number of operands

        void local(int, int);

        Instruction(Bytecode w, int o = 1): which(w), operands(o) {
            registers = new void*[o];
            locals = new int[o];
        }
};

#endif
