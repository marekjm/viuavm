#ifndef VIUA_TYPE_CLOSURE_H
#define VIUA_TYPE_CLOSURE_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "../cpu/registerset.h"
#include "function.h"


class Closure : public Function {
    /** Closure type.
     */
    public:
        RegisterSet* regset;

        std::string function_name;

        virtual std::string type() const;
        virtual std::string str() const;
        virtual std::string repr() const;

        virtual bool boolean() const;

        virtual Type* copy() const;

        // FIXME: implement real dtor
        Closure();
        virtual ~Closure();
};


#endif
