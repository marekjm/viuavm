/*
 *  Copyright (C) 2016 Marek Marecki
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

#ifndef VIUA_TYPES_NUMBER_H
#define VIUA_TYPES_NUMBER_H

#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <viua/types/type.h>


namespace viua {
    using float32 = float;
    using float64 = double;

    namespace types {
        namespace numeric {
            class Number : public Type {
                /** Base number type.
                 *
                 *  All types representing numbers *must* inherit from
                 *  this class.
                 *  Otherwise, they will not be usable by arithmetic instructions.
                 */
                public:
                    std::string type() const override {
                        return "Number";
                    }
                    std::string str() const override = 0;
                    bool boolean() const override = 0;

                    virtual bool negative() {
                        return (as_int64() < 0);
                    }

                    virtual int8_t as_int8() const = 0;
                    virtual int16_t as_int16() const = 0;
                    virtual int32_t as_int32() const = 0;
                    virtual int64_t as_int64() const = 0;

                    virtual uint8_t as_uint8() const = 0;
                    virtual uint16_t as_uint16() const = 0;
                    virtual uint32_t as_uint32() const = 0;
                    virtual uint64_t as_uint64() const = 0;

                    virtual float32 as_float32() const = 0;
                    virtual float64 as_float64() const = 0;

                    Number() {}
                    virtual ~Number() {}
            };
        }
    }
}


#endif
