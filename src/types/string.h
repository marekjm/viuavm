#ifndef TATANKA_TYPES_BYTE_H
#define TATANKA_TYPES_BYTE_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


std::string encode(const std::string&);


class String : public Object {
    std::string _value;

    public:
        std::string type() const {
            return "String";
        }
        std::string str() const {
            std::ostringstream s;
            s << _value;
            return s.str();
        }
        bool boolean() const {
            return _value.size() != 0;
        }

        std::string& value() { return _value; }

        String(std::string s = ""): _value(s) {}
};


#endif
