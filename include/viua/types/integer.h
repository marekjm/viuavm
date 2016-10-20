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

#ifndef VIUA_TYPES_INTEGER_H
#define VIUA_TYPES_INTEGER_H

#pragma once

#include <string>
#include <sstream>
#include <viua/types/type.h>


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


#endif
