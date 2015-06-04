#ifndef VIUA_TYPE_FUNCTION_H
#define VIUA_TYPE_FUNCTION_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "../cpu/registerset.h"
#include "type.h"


class Function : public Type {
    /** Type representing a function.
     */
    public:
        Type** arguments;
        bool* argreferences;
        int arguments_size;

        std::string function_name;

        std::string type() const;
        std::string str() const;
        std::string repr() const;

        bool boolean() const;

        Type* copy() const;

        // FIXME: implement real dtor
        Function();
        ~Function();
};


#endif
