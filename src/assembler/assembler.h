#ifndef WUDOO_ASSEMBLER_H
#define WUDOO_ASSEMBLER_H

#pragma once


#include <string>
#include <tuple>
#include "../program.h"

namespace assembler {
    namespace operands {
        int_op getint(const std::string& s);
        byte_op getbyte(const std::string& s);
        float_op getfloat(const std::string& s);

        std::tuple<std::string, std::string> get2(std::string s);
        std::tuple<std::string, std::string, std::string> get3(std::string s, bool fill_third = true);
    }

    namespace getters {
    }
}


#endif
