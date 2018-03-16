/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <ios>
#include <sstream>
#include <string>
#include <viua/types/number.h>


namespace viua {
    namespace types {
        class Float : public viua::types::numeric::Number {
            /** Basic integer type.
             *  It is suitable for mathematical operations.
             */
          public:
            using underlying_type = double;

          private:
            underlying_type number;

          public:
            static const std::string type_name;

            std::string type() const override;
            std::string str() const override;
            bool boolean() const override;

            auto value() -> decltype(number) &;

            std::unique_ptr<Value> copy() const override;

            auto as_integer() const -> int64_t override;
            auto as_float() const -> float64 override;

            auto operator+(const Number&) const -> std::unique_ptr<Number> override;
            auto operator-(const Number&) const -> std::unique_ptr<Number> override;
            auto operator*(const Number&)const -> std::unique_ptr<Number> override;
            auto operator/(const Number&) const -> std::unique_ptr<Number> override;

            auto operator<(const Number&) const -> std::unique_ptr<Boolean> override;
            auto operator<=(const Number&) const -> std::unique_ptr<Boolean> override;
            auto operator>(const Number&) const -> std::unique_ptr<Boolean> override;
            auto operator>=(const Number&) const -> std::unique_ptr<Boolean> override;
            auto operator==(const Number&) const -> std::unique_ptr<Boolean> override;

            Float(decltype(number) n = 0);
        };
    }  // namespace types
}  // namespace viua


#endif
