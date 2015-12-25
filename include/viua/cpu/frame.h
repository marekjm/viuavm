#ifndef VIUA_CPU_FRAME_H
#define VIUA_CPU_FRAME_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "registerset.h"

class Frame {
        bool deallocate_arguments;
    public:
        byte* return_address;
        RegisterSet* args;
        RegisterSet* regset;

        int place_return_value_in;
        bool resolve_return_value_register;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        void captureArguments();

        Frame(byte* ra, long unsigned argsize, long unsigned regsize = 16):
            deallocate_arguments(false),
            return_address(ra),
            args(nullptr), regset(nullptr),
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
            // drop all pointers in arguments registers set
            // to prevent double deallocation
            if (not deallocate_arguments) {
                args->drop();
            }

            delete args;
            delete regset;
        }
};


#endif
