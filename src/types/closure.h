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

        /*
        Closure(const std::string& fn):
            arguments(0), argreferences(0), arguments_size(0),
            registers(0), references(0), registers_size(0),
            function_name(fn) {
        }
        */
        // FIXME: implement real dtor
        Closure();
        ~Closure();
};


#endif
