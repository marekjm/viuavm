#ifndef VIUA_DISASSEMBLER_H
#define VIUA_DISASSEMBLER_H

#pragma once

#include <string>
#include <tuple>

namespace disassembler {
    std::tuple<std::string, unsigned> instruction(byte*);
};


#endif
