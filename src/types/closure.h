#ifndef WUDOO_TYPE_CLOSURE_H
#define WUDOO_TYPE_CLOSURE_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "object.h"
#include "string.h"


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

        std::string type() const {
            return "Closure";
        }
        std::string str() const;
        bool boolean() const {
            return true;
        }

        Object* copy() const {
            Closure* clsr = new Closure(function_name);
            // FIXME: we should copy the registers instead of just pointing to them
            // FIXME: for the above one, copy ctor would be nice
            clsr->registers = registers;
            clsr->references = references;
            clsr->registers_size = registers_size;
            return clsr;
        }

        Object* value() { return new String(function_name); }

        Closue(): registers(0), references(0), registers_size(0), function_name("") {}
        Closure(const std::string& fn): registers(0), references(0), registers_size(0), function_name(fn) {}
        // FIXME: implement real dtor
        ~Closure() {};
};


#endif
