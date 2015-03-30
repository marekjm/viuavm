#ifndef WUDOO_TYPE_CLOSURE_H
#define WUDOO_TYPE_CLOSURE_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "object.h"


class Closure : public Object {
    /** Vector type.
     */
    public:
        Object** arguments;
        bool* argreferences;
        int arguments_size;

        Object** registers;
        bool* references;
        int registers_size;

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
