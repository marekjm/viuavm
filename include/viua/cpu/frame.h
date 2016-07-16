/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIUA_CPU_FRAME_H
#define VIUA_CPU_FRAME_H

#pragma once

#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/cpu/registerset.h>

class Frame {
        bool owns_local_register_set;
    public:
        byte* return_address;
        RegisterSet* args;
        RegisterSet* regset;

        unsigned place_return_value_in;
        bool resolve_return_value_register;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        void setLocalRegisterSet(RegisterSet*, bool receives_ownership = true);

        Frame(byte* ra, long unsigned argsize, long unsigned regsize = 16):
            owns_local_register_set(true),
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
            if (args != nullptr) {
                delete args;
            }

            if (owns_local_register_set) {
                delete regset;
            }
        }
};


#endif
