/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIUA_TYPES_BYTE_H
#define VIUA_TYPES_BYTE_H

#pragma once

#include <string>
#include <sstream>
#include <viua/types/type.h>


class Byte : public Type {
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

        Type* copy() const {
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
