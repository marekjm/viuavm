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
#include <viua/util/memory.h>
#include <viua/kernel/registerset.h>

class Frame {
    public:
        byte* return_address;
        std::unique_ptr<viua::kernel::RegisterSet> arguments;
        viua::util::memory::maybe_unique_ptr<viua::kernel::RegisterSet> local_register_set;

        bool return_void;
        unsigned place_return_value_in;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        void setLocalRegisterSet(viua::kernel::RegisterSet*, bool receives_ownership = true);

        Frame(byte*, long unsigned, long unsigned = 16);
        Frame(const Frame&);
};


#endif
