/*
 *  Copyright (C) 2016 Marek Marecki
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

#ifndef VIUA_BYTECODE_DECODER_OPERANDS_H
#define VIUA_BYTECODE_DECODER_OPERANDS_H

#pragma once

#include <string>
#include <utility>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>

// forward declaration for functions declared in this file
class Process;

namespace viua {
    namespace bytecode {
        namespace decoder {
            namespace operands {
                auto fetch_register_index(byte*, Process*) -> std::tuple<byte*, unsigned>;
                auto fetch_primitive_char(byte*, Process*) -> std::tuple<byte*, char>;
                auto fetch_primitive_uint(byte*, Process*) -> std::tuple<byte*, unsigned>;
                auto fetch_primitive_uint64(byte*, Process*) -> std::tuple<byte*, uint64_t>;
                auto fetch_primitive_int(byte*, Process*) -> std::tuple<byte*, int>;
                auto fetch_atom(byte*, Process*) -> std::tuple<byte*, std::string>;
                auto fetch_object(byte*, Process*) -> std::tuple<byte*, Type*>;

                auto extract_primitive_uint64(byte*, Process*) -> uint64_t;
            }
        }
    }
}


#endif
