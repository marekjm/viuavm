#ifndef TATANKA_TYPES_BOOLEAN_H
#define TATANKA_TYPES_BOOLEAN_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


class Boolean : public Object {
    bool boolean;

    public:
        std::string type() const {
            return "Boolean";
        }
        std::string str() const {
            return ( boolean ? "true" : "false" );
        }

        bool& value() { return boolean; }

        Boolean(bool b = false): boolean(b) {}
};


#endif
