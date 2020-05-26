/*
 *  Copyright (C) 2016, 2017, 2020 Marek Marecki
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

#include <sstream>
#include <string>
#include <viua/exceptions.h>
#include <viua/types/boolean.h>
#include <viua/types/float.h>
using namespace viua::types;

std::string const viua::types::Float::type_name = "Float";


std::string Float::type() const { return type_name; }
std::string Float::str() const { return std::to_string(number); }
bool Float::boolean() const { return (number != 0); }

auto Float::value() -> decltype(number)& { return number; }

std::unique_ptr<viua::types::Value> Float::copy() const
{
    return std::make_unique<Float>(number);
}

auto Float::as_integer() const -> int64_t
{
    return static_cast<int64_t>(number);
}

auto Float::as_float() const -> viua::float64 { return number; }

auto Float::operator+(numeric::Number const& that) const
    -> std::unique_ptr<numeric::Number>
{
    return std::make_unique<Float>(number + that.as_float());
}
auto Float::operator-(numeric::Number const& that) const
    -> std::unique_ptr<numeric::Number>
{
    return std::make_unique<Float>(number - that.as_float());
}
auto Float::operator*(numeric::Number const& that) const
    -> std::unique_ptr<numeric::Number>
{
    return std::make_unique<Float>(number * that.as_float());
}
auto Float::operator/(numeric::Number const& that) const
    -> std::unique_ptr<numeric::Number>
{
    if (that.as_integer() == 0) {
        throw viua::util::exceptions::make_unique_exception<
            viua::runtime::exceptions::Zero_division>();
    }
    return std::make_unique<Float>(number / that.as_float());
}

auto Float::operator<(numeric::Number const& that) const
    -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(number < that.as_float());
}
auto Float::operator<=(numeric::Number const& that) const
    -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(number <= that.as_float());
}
auto Float::operator>(numeric::Number const& that) const
    -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(number > that.as_float());
}
auto Float::operator>=(numeric::Number const& that) const
    -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(number >= that.as_float());
}
auto Float::operator==(numeric::Number const& that) const
    -> std::unique_ptr<Boolean>
{
    return std::make_unique<Boolean>(number == that.as_float());
}

Float::Float(decltype(number) n) : number(n) {}
