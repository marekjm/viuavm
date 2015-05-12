#ifndef VIUA_CPU_CATCHER_H
#define VIUA_CPU_CATCHER_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"

class Catcher {
    public:
        byte* block_address;
        std::string caught_type;

        Catcher(byte* ba, const std::string& ct): block_address(ba), caught_type(ct) {}
};


#endif
