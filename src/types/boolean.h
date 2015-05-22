#ifndef VIUA_TYPES_BOOLEAN_H
#define VIUA_TYPES_BOOLEAN_H

#pragma once

#include <string>
#include <sstream>
#include "integer.h"


class Boolean : public Integer {
    /** Boolean object.
     *
     *  This type is used to hold true and false values.
     */
    bool b;

    public:
        std::string type() const {
            return "Boolean";
        }
        std::string str() const {
            return ( b ? "true" : "false" );
        }
        bool boolean() const {
            return b;
        }

        bool& value() { return b; }

        // Integer methods
        int as_integer() const { return int(b); }
        int increment() { return (b = true); }
        int decrement() { return (b = false); }

        Type* copy() const {
            return new Boolean(b);
        }

        Boolean(bool v = false): b(v) {}
};


#endif
