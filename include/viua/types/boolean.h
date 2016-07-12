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

        virtual std::vector<std::string> bases() const {
            return std::vector<std::string>{"Integer"};
        }
        virtual std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"Integer", "Type"};
        }

        Type* copy() const {
            return new Boolean(b);
        }

        Boolean(bool v = false): b(v) {}
};


#endif
