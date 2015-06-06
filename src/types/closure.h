#ifndef VIUA_TYPE_CLOSURE_H
#define VIUA_TYPE_CLOSURE_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "../cpu/registerset.h"
#include "type.h"


class Closure : public Type {
    /** Closure type.
     */
    public:
        RegisterSet* regset;

        std::string function_name;

        std::string type() const;
        std::string str() const;
        std::string repr() const;

        bool boolean() const;

        Type* copy() const;

        // FIXME: implement real dtor
        Closure();
        ~Closure();
};


#endif
