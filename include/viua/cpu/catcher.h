#ifndef VIUA_CPU_CATCHER_H
#define VIUA_CPU_CATCHER_H

#pragma once

#include <string>
#include <viua/bytecode/bytetypedef.h>

class Catcher {
    public:
        std::string caught_type;
        std::string catcher_name;

        Catcher(const std::string& type_name, const std::string& catching_block): caught_type(type_name), catcher_name(catching_block) {}
};


#endif
