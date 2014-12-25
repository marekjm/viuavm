#ifndef TATANKA_CPU_H
#define TATANKA_CPU_H

#pragma once

#include <cstdint>
#include "bytecode.h"
#include "types/object.h"
#include "types/integer.h"

const int DEFAULT_REGISTER_SIZE = 256;


class CPU {
    char* bytecode;
    uint16_t bytecode_size;

    Object** registers;
    int reg_count;

    Object* fetchRegister(int i, bool nullok = false);

    char* istore(char*);
    char* print(char*);
    char* echo(char*);

    public:
        bool debug;

        CPU& load(char*);
        CPU& bytes(uint16_t);
        int run();

        CPU(int r = DEFAULT_REGISTER_SIZE): bytecode(0), bytecode_size(0), registers(0), reg_count(r), debug(false) {
            /*  Basic constructor.
             *  Creates registers array of requested size and
             *  initializes it with zeroes.
             */
            registers = new Object*[reg_count];
            for (int i = 0; i < reg_count; ++i) { registers[i] = 0; }
        }

        ~CPU() {
            /*  Destructor must free all memory allocated for values stored in registers.
             *  Here we iterate over all registers and delete non-null pointers.
             *
             *  Destructor also frees memory at bytecode pointer so make sure you gave CPU a copy of the bytecode if you want to keep it
             *  after the CPU is finished.
             */
            for (int i = 0; i < reg_count; ++i) {
                if (registers[i]) {
                    delete registers[i];
                }
            }
            delete[] registers;
            if (bytecode) { delete[] bytecode; }
        }
};

#endif
