#ifndef VIUA_TYPE_CLOSURE_H
#define VIUA_TYPE_CLOSURE_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "../cpu/registerset.h"
#include "object.h"


class Closure : public Object {
    /** Vector type.
     */
    public:
        Object** arguments;
        bool* argreferences;
        int arguments_size;

        RegisterSet* regset;

        std::string function_name;

        std::string type() const;
        std::string str() const;
        std::string repr() const;

        bool boolean() const;

        Object* copy() const;

        // FIXME: implement real dtor
        Closure();
        ~Closure();
};


#endif
