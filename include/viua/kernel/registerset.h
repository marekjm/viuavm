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
#include <viua/types/type.h>

typedef unsigned char mask_t;
typedef long unsigned registerset_size_type;


enum REGISTER_MASKS: mask_t {
    REFERENCE       = (1 << 0),
    COPY_ON_WRITE   = (1 << 1),
    KEEP            = (1 << 2), // do not delete when frame is popped from stack
    BIND            = (1 << 3), // hint for closure instruction what registers to bind
    BOUND           = (1 << 4), // markes registers bound in closures
    PASSED          = (1 << 5), // marks registers containing objects passed to functions by parameter
                                // these registers *MUST NOT* be overwritten
    MOVED           = (1 << 6), // marks registers containing moved parameters
};


namespace viua {
    namespace kernel {
        class RegisterSet {
            registerset_size_type registerset_size;
            std::vector<std::unique_ptr<viua::types::Type>> registers;
            std::vector<mask_t>  masks;

            public:
                // basic access to registers
                viua::types::Type* put(registerset_size_type, viua::types::Type*);
                viua::types::Type* pop(registerset_size_type);
                viua::types::Type* set(registerset_size_type, viua::types::Type*);
                viua::types::Type* get(registerset_size_type);
                viua::types::Type* at(registerset_size_type);

                // register modifications
                void move(registerset_size_type, registerset_size_type);
                void swap(registerset_size_type, registerset_size_type);
                void empty(registerset_size_type);
                void free(registerset_size_type);

                // mask inspection and manipulation
                void flag(registerset_size_type, mask_t);
                void unflag(registerset_size_type, mask_t);
                void clear(registerset_size_type);
                bool isflagged(registerset_size_type, mask_t);
                void setmask(registerset_size_type, mask_t);
                mask_t getmask(registerset_size_type);

                void drop();
                inline registerset_size_type size() { return registerset_size; }

                std::unique_ptr<RegisterSet> copy();

                RegisterSet(registerset_size_type sz);
                ~RegisterSet();
        };
    }
}


#endif
