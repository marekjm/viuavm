#ifndef TATANKA_CPU_H
#define TATANKA_CPU_H

#pragma once

#include <vector>
#include "bytecode.h"

class CPU {
    int instruction_ptr = 0;

    std::vector<Instruction> instructions;
    std::vector<void*> registers;

    void* getRegister(int);

    public:
        void setRegisterCount(int n);

        void load(std::vector<Instruction> ins);
        int run(int cycles = 0);

        CPU() {}
};

#endif
