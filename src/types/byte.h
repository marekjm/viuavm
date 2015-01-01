#ifndef TATANKA_TYPES_BYTE_H
#define TATANKA_TYPES_BYTE_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


class Byte : public Object {
    char byte_;

    public:
        std::string type() const {
            return "Byte";
        }
        std::string str() const {
            std::ostringstream s;
            s << byte_;
            return s.str();
        }
        bool boolean() const {
            return _char != 0;
        }

        char& value() { return byte_; }

        Byte(char b = 0): byte_(b) {}
};


class UnsignedByte : public Byte {
    unsigned char ubyte_;

    public:
        std::string type() const {
            return "UnsignedByte";
        }
        std::string str() const {
            std::ostringstream s;
            s << ubyte_;
            return s.str();
        }
        bool boolean() const { return ubyte_ != 0; }

        unsigned char& value() { return ubyte_; }

        UnsignedByte(unsigned char b = 0): ubyte_(b) {}
};


#endif
