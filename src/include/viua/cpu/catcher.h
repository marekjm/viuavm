#ifndef VIUA_CPU_CATCHER_H
#define VIUA_CPU_CATCHER_H

#pragma once

#include <string>
#include <viua/bytecode/bytetypedef.h>

class Catcher {
    public:
        std::string caught_type;
        std::string catcher_name;
        byte* block_address;

        Catcher(const std::string& ct, const std::string& cn, byte* ba): caught_type(ct), catcher_name(cn), block_address(ba) {}
};


#endif
