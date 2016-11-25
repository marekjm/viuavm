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
#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/registerset.h>

class Frame {
        bool owns_local_register_set;
    public:
        byte* return_address;
        std::unique_ptr<viua::kernel::RegisterSet> args;
        std::unique_ptr<viua::kernel::RegisterSet> regset;

        bool return_void;
        unsigned place_return_value_in;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        void setLocalRegisterSet(viua::kernel::RegisterSet*, bool receives_ownership = true);

        Frame(byte* ra, long unsigned argsize, long unsigned regsize = 16):
            owns_local_register_set(true),
            return_address(ra),
            args(nullptr), regset(nullptr),
            return_void(false),
            place_return_value_in(0)
        {
            args.reset(new viua::kernel::RegisterSet(argsize));
            regset.reset(new viua::kernel::RegisterSet(regsize));
        }
        Frame(const Frame& that) {
            return_address = that.return_address;

            // FIXME: copy the registers maybe?
            // FIXME: oh, and the arguments too, while you're at it!
        }
        ~Frame() {
            if (not owns_local_register_set) {
                // FIXME use std::shared_ptr<> maybe?
                regset.release();
            }
        }
};


#endif
