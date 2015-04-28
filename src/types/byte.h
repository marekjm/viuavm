#ifndef VIUA_TYPES_BYTE_H
#define VIUA_TYPES_BYTE_H

#pragma once

#include <string>
#include <sstream>
#include "object.h"


class Byte : public Object {
    /** Byte type can be used to hold generic data or
     *  act as known-from-C char type.
     *
     *  The second usage is however discouraged.
     *  There are String objects to hold letters.
     */
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
            return byte_ != 0;
        }

        char& value() { return byte_; }

        Object* copy() const {
            return new Byte(byte_);
        }

        Byte(char b = 0): byte_(b) {}
};


class UnsignedByte : public Byte {
    /** Unsigned variant of Byte type.
     */
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
