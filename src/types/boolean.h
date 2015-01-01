#ifndef TATANKA_TYPES_BOOLEAN_H
#define TATANKA_TYPES_BOOLEAN_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


class Boolean : public Object {
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

        Boolean(bool v = false): b(v) {}
};


#endif
