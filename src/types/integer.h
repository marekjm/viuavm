#ifndef TATANKA_TYPES_INTEGER_H
#define TATANKA_TYPES_INTEGER_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


class Integer : public Object {
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

        int& value() { return number; }

        Integer(int n = 0): number(n) {}
};


class UnsignedInteger : public Integer {
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

        unsigned value() { return number; }

        UnsignedInteger(unsigned n = 0): number(n) {}
};


#endif
