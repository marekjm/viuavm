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

#ifndef VIUA_CPU_OPEX_H
#define VIUA_CPU_OPEX_H

#pragma once

#include <viua/bytecode/bytetypedef.h>
#include <viua/support/pointer.h>

namespace viua {
    namespace kernel {
        namespace util {
            void extractIntegerOperand(byte*& instruction_stream, bool& boolean, int& integer);
            void extractFloatingPointOperand(byte*& instruction_stream, float& fp);

            template <class T> void extractOperand(byte*& instruction_stream, T& operand) {
                T ex_operand;
                ex_operand = *(reinterpret_cast<T*>(instruction_stream));
                pointer::inc<T, byte>(instruction_stream);
                operand = ex_operand;
            }
        }
    }
}

#endif
