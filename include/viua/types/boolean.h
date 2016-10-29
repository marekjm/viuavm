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
#include <viua/types/integer.h>


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
            return std::vector<std::string>{"Integer", "Number", "Type"};
        }

        Type* copy() const {
            return new Boolean(b);
        }

        int8_t as_int8() const override { return b; }
        int16_t as_int16() const override { return b; }
        int32_t as_int32() const override { return b; }
        int64_t as_int64() const override { return b; }

        uint8_t as_uint8() const override { return b; }
        uint16_t as_uint16() const override { return b; }
        uint32_t as_uint32() const override { return b; }
        uint64_t as_uint64() const override { return b; }

        viua::float32 as_float32() const override { return b; }
        viua::float64 as_float64() const override { return b; }

        Boolean(bool v = false): b(v) {}
};


#endif
