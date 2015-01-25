#ifndef WUDOO_CPU_FRAME_H
#define WUDOO_CPU_FRAME_H

#pragma once

#include "../bytecode/bytetypedef.h"


class Frame {
    public:
    byte* return_address;
    int arguments_size;
    Object** arguments;

    Object** registers;
    bool* references;
    int registers_size;

    inline byte* ret_address() { return return_address; }

    Frame(byte* ra, int argsize, int regsize = 64): return_address(ra), arguments_size(argsize), arguments(new Object*[argsize]), registers(new Object*[regsize]), references(new bool[regsize]), registers_size(regsize) {
        for (int i = 0; i < argsize; ++i) { arguments[i] = 0; }
        for (int i = 0; i < regsize; ++i) { registers[i] = 0; }
        for (int i = 0; i < regsize; ++i) { references[i] = false; }
    }
    Frame(const Frame& that) {
        return_address = that.return_address;
        registers_size = that.registers_size;
        for (int i = 0; i < registers_size; ++i) {
            if (that.registers[i] != 0) {
                registers[i] = that.registers[i]->copy();
            } else {
                registers[i] = 0;
            }
        }
        registers_size = that.registers_size;
        for (int i = 0; i < registers_size; ++i) {
            if (that.registers[i] != 0) {
                registers[i] = that.registers[i]->copy();
            } else {
                registers[i] = 0;
            }
        }
    }
    ~Frame() {
        for (int i = 0; i < arguments_size; ++i) {
            if (arguments[i] != 0) { delete arguments[i]; }
        }
        delete[] arguments;
        for (int i = 0; i < registers_size; ++i) {
            if (registers[i] != 0 and !references) { delete registers[i]; }
        }
        delete[] registers;
    }
};


#endif
