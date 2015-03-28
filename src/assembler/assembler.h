#ifndef WUDOO_ASSEMBLER_H
#define WUDOO_ASSEMBLER_H

#pragma once


#include "../program.h"

namespace assembler {
    namespace operands {
        int_op getint(const std::string& s);
        byte_op getbyte(const std::string& s);
        float_op getfloat(const std::string& s);
    }

    namespace getters {
    }
}


#endif
