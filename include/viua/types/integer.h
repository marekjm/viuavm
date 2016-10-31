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
#include <viua/types/number.h>


namespace viua {
    namespace types {
        class Integer : public viua::types::numeric::Number {
            /** Basic integer type.
             *  It is suitable for mathematical operations.
             */
            int number;

            public:
                std::string type() const override;
                std::string str() const override;
                bool boolean() const override;

                int& value();

                virtual int increment();
                virtual int decrement();

                Type* copy() const override;

                int8_t as_int8() const override;
                int16_t as_int16() const override;
                int32_t as_int32() const override;
                int64_t as_int64() const override;

                uint8_t as_uint8() const override;
                uint16_t as_uint16() const override;
                uint32_t as_uint32() const override;
                uint64_t as_uint64() const override;

                viua::float32 as_float32() const override;
                viua::float64 as_float64() const override;

                Integer(int n = 0): number(n) {}
        };
    }
}


#endif
