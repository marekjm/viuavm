#ifndef WUDOO_CPU_FRAME_H
#define WUDOO_CPU_FRAME_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "registerset.h"

class Frame {
    public:
        byte* return_address;
        int arguments_size;
        Object** arguments;
        bool* argreferences;

        RegisterSet* args;
        RegisterSet* regset;

        int place_return_value_in;
        bool resolve_return_value_register;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        Frame(byte* ra, int argsize, int regsize = 16):
            return_address(ra),
            arguments_size(argsize), arguments(0), argreferences(0),
            args(0), regset(0),
            place_return_value_in(0), resolve_return_value_register(false)
        {
            if (argsize > 0) {
                arguments = new Object*[argsize];
                argreferences = new bool[argsize];
            }
            for (int i = 0; i < argsize; ++i) { arguments[i] = 0; }
            for (int i = 0; i < argsize; ++i) { argreferences[i] = false; }
            regset = new RegisterSet(regsize);
        }
        Frame(const Frame& that) {
            return_address = that.return_address;

            // FIXME: copy the registers maybe?

            arguments_size = that.arguments_size;
            for (int i = 0; i < arguments_size; ++i) {
                if (that.arguments[i] != 0) {
                    arguments[i] = that.arguments[i]->copy();
                } else {
                    arguments[i] = 0;
                }
            }
            for (int i = 0; i < arguments_size; ++i) {
                argreferences[i] = that.argreferences[i];
            }
        }
        ~Frame() {
            for (int i = 0; i < arguments_size; ++i) {
                if (arguments[i] != 0 and !argreferences[i]) { delete arguments[i]; }
            }
            if (arguments_size) {
                delete[] arguments;
                delete[] argreferences;
            }

            delete regset;
        }
};


#endif
