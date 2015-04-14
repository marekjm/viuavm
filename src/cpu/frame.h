#ifndef WUDOO_CPU_FRAME_H
#define WUDOO_CPU_FRAME_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "registerset.h"

class Frame {
    public:
        byte* return_address;
        RegisterSet* args;
        RegisterSet* regset;

        int place_return_value_in;
        bool resolve_return_value_register;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        Frame(byte* ra, int argsize, int regsize = 16):
            return_address(ra),
            args(0), regset(0),
            place_return_value_in(0), resolve_return_value_register(false)
        {
            args = new RegisterSet(argsize);
            regset = new RegisterSet(regsize);
        }
        Frame(const Frame& that) {
            return_address = that.return_address;

            // FIXME: copy the registers maybe?
            // FIXME: oh, and the arguments too, while you're at it!
        }
        ~Frame() {
            delete args;
            delete regset;
        }
};


#endif
