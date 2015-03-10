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
        int arguments_size;
        Object** arguments;
        bool* argreferences;

        Object** registers;
        bool* references;
        int registers_size;

        std::string function_name;

        inline byte* ret_address() { return return_address; }

        std::string type() const {
            return "Closure";
        }
        std::string str() const;
        bool boolean() const {
            return true;
        }

        Object* copy() const {
            return new Closure();
        }

        std::vector<Object*>& value() { return this; }

        Closure(const std::string& fn): function_name(fn), registers(0), references(0), registers_size(0) {}
};


#endif
