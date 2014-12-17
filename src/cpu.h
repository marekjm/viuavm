#ifndef TATANKA_CPU_H
#define TATANKA_CPU_H

#pragma once

#include "bytecode.h"

const int DEFAULT_REGISTER_SIZE = 64;


class CPU {
    char* bytecode;
    void** registers;
    int reg_count;

    void* getRegister(int);

    public:
        CPU& load(char*);
        int run(int cycles = 0);

        CPU(int r = DEFAULT_REGISTER_SIZE): reg_count(r) {
            /*  Basic constructor.
             *  Creates registers array of requested size and
             *  initializes it with zeroes.
             */
            registers = new void*[reg_count];
            for (int i = 0; i < reg_count; ++i) { registers[0] = 0; }
            bytecode = 0;
        }

        ~CPU() {
            /*  Destructor must free all memory allocated for values stored in registers.
             *  Here we iterate over all registers and delete non-null pointers.
             */
            for (int i = 0; i < reg_count; ++i) {
                if (registers[i]) {
                    delete registers[i];
                }
            }
        }
};

#endif
