#ifndef VIUA_CPU_OPEX_H
#define VIUA_CPU_OPEX_H

#pragma once

#include <viua/bytecode/bytetypedef.h>

namespace viua {
    namespace cpu {
        namespace util {
            void extractIntegerOperand(byte*& instruction_stream, bool& boolean, int& integer);
        }
    }
}

#endif
