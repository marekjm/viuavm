#ifndef TATANKA_BYTECODES_H
#define TATANKA_BYTECODES_H

#pragma once

#include <vector>

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

        static Instruction instance(Bytecode w, int o);

        Instruction local(int, int);

        Instruction(Bytecode w, int o = 1): which(w), operands(o) {
            registers = new void*[o];
            locals = new int[o];
        }
};


class Program {
    std::vector<Instruction> instructions;

    public:
        Program& push(Instruction i);
        std::vector<Instruction> getInstructionVector();
};

#endif
