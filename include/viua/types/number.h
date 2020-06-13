/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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

#include <cstdint>
#include <sstream>
#include <string>

#include <viua/types/boolean.h>
#include <viua/types/value.h>


namespace viua {
using float32 = float;
using float64 = double;

namespace types { namespace numeric {
class Number : public Value {
    /** Base number type.
     *
     *  All types representing numbers *must* inherit from
     *  this class.
     *  Otherwise, they will not be usable by arithmetic instructions.
     */
  public:
    constexpr static auto type_name = "Number";

    std::string type() const override;
    std::string str() const override = 0;
    bool boolean() const override    = 0;

    virtual bool negative() const;

    virtual auto as_integer() const -> int64_t = 0;
    virtual auto as_float() const -> float64   = 0;

    virtual auto operator+(Number const&) const -> std::unique_ptr<Number> = 0;
    virtual auto operator-(Number const&) const -> std::unique_ptr<Number> = 0;
    virtual auto operator*(Number const&) const -> std::unique_ptr<Number> = 0;
    virtual auto operator/(Number const&) const -> std::unique_ptr<Number> = 0;

    virtual auto operator<(Number const&) const -> std::unique_ptr<Boolean> = 0;
    virtual auto operator<=(Number const&) const
        -> std::unique_ptr<Boolean>                                         = 0;
    virtual auto operator>(Number const&) const -> std::unique_ptr<Boolean> = 0;
    virtual auto operator>=(Number const&) const
        -> std::unique_ptr<Boolean> = 0;
    virtual auto operator==(Number const&) const
        -> std::unique_ptr<Boolean> = 0;

    Number();
    virtual ~Number();
};
}}  // namespace types::numeric
}  // namespace viua


#endif
