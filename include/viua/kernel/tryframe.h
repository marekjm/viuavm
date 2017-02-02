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

#ifndef VIUA_CPU_TRYFRAME_H
#define VIUA_CPU_TRYFRAME_H

#pragma once

#include <string>
#include <map>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/catcher.h>

class TryFrame {
    public:
        viua::internals::types::byte* return_address;
        Frame* associated_frame;

        std::string block_name;

        std::map<std::string, Catcher*> catchers;

        inline viua::internals::types::byte* ret_address() { return return_address; }

        TryFrame(): return_address(nullptr), associated_frame(nullptr) {}
        ~TryFrame() {
            for (auto p : catchers) {
                delete p.second;
            }
        }
};


#endif
