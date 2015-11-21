#ifndef VIUA_CPU_OPEX_H
#define VIUA_CPU_OPEX_H

#pragma once

#include <viua/bytecode/bytetypedef.h>
#include <viua/support/pointer.h>

namespace viua {
    namespace cpu {
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
