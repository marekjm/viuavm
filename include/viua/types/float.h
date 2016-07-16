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

#ifndef VIUA_TYPES_FLOAT_H
#define VIUA_TYPES_FLOAT_H

#pragma once

#include <string>
#include <ios>
#include <sstream>
#include <viua/types/type.h>


class Float : public Type {
    /** Basic integer type.
     *  It is suitable for mathematical operations.
     */
    float data;

    public:
        std::string type() const {
            return "Float";
        }
        std::string str() const {
            std::ostringstream s;
            // std::fixed because 1.0 will yield '1' and not '1.0' when stringified
            s << std::fixed << data;
            return s.str();
        }
        bool boolean() const { return data != 0; }

        float& value() { return data; }

        Type* copy() const {
            return new Float(data);
        }

        Float(float n = 0): data(n) {}
};


#endif
