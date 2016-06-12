#ifndef VIUA_TYPES_INTEGER_H
#define VIUA_TYPES_INTEGER_H

#pragma once

#include <string>
#include <sstream>
#include "type.h"


class Integer : public Type {
    /** Basic integer type.
     *  It is suitable for mathematical operations.
     */
    int number;

    public:
        std::string type() const {
            return "Integer";
        }
        std::string str() const {
            std::ostringstream s;
            s << number;
            return s.str();
        }
        bool boolean() const { return number != 0; }

        int& value() { return number; }

        virtual int as_integer() const { return number; }
        virtual unsigned as_unsigned() const { return static_cast<unsigned>(number); }
        virtual int increment() { return (++number); }
        virtual int decrement() { return (--number); }

        Type* copy() const {
            return new Integer(number);
        }

        Integer(int n = 0): number(n) {}
};


class UnsignedInteger : public Integer {
    /** Unsigned variant of Integer type.
     */
    unsigned number;

    public:
        std::string type() const {
            return "UnsignedInteger";
        }
        std::string str() const {
            std::ostringstream s;
            s << number;
            return s.str();
        }
        bool boolean() const { return number != 0; }

        unsigned value() { return number; }

        UnsignedInteger(unsigned n = 0): number(n) {}
};


#endif
