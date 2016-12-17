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

#ifndef VIUA_REGISTERSET_H
#define VIUA_REGISTERSET_H

#pragma once

#include <memory>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>

typedef unsigned char mask_t;


enum REGISTER_MASKS: mask_t {
    COPY_ON_WRITE   = (1 << 0),
    MOVED           = (1 << 1), // marks registers containing moved parameters
};


namespace viua {
    namespace kernel {
        class RegisterSet {
            viua::internals::types::register_index registerset_size;
            std::vector<std::unique_ptr<viua::types::Type>> registers;
            std::vector<mask_t>  masks;

            public:
                // basic access to registers
                void put(viua::internals::types::register_index, std::unique_ptr<viua::types::Type>);
                std::unique_ptr<viua::types::Type> pop(viua::internals::types::register_index);
                void set(viua::internals::types::register_index, std::unique_ptr<viua::types::Type>);
                viua::types::Type* get(viua::internals::types::register_index);
                viua::types::Type* at(viua::internals::types::register_index);

                // register modifications
                void move(viua::internals::types::register_index, viua::internals::types::register_index);
                void swap(viua::internals::types::register_index, viua::internals::types::register_index);
                void empty(viua::internals::types::register_index);
                void free(viua::internals::types::register_index);

                // mask inspection and manipulation
                void flag(viua::internals::types::register_index, mask_t);
                void unflag(viua::internals::types::register_index, mask_t);
                void clear(viua::internals::types::register_index);
                bool isflagged(viua::internals::types::register_index, mask_t);
                void setmask(viua::internals::types::register_index, mask_t);
                mask_t getmask(viua::internals::types::register_index);

                void drop();
                inline viua::internals::types::register_index size() { return registerset_size; }

                std::unique_ptr<RegisterSet> copy();

                RegisterSet(viua::internals::types::register_index sz);
                ~RegisterSet();
        };
    }
}


#endif
