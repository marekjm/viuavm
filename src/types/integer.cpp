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

#include <string>
#include <sstream>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
using namespace std;
using namespace viua::types;

const string viua::types::Integer::type_name = "Integer";

string Integer::type() const {
    return "Integer";
}
string Integer::str() const {
    ostringstream s;
    s << number;
    return s.str();
}
bool Integer::boolean() const {
    return (number != 0);
}

auto Integer::value() -> decltype(number) { return number; }

int64_t Integer::increment() {
    return (++number);
}
int64_t Integer::decrement() {
    return (--number);
}

unique_ptr<Value> Integer::copy() const {
    return unique_ptr<Value>{new Integer(number)};
}

auto Integer::as_integer() const -> int64_t {
    return number;
}

auto Integer::as_float() const -> viua::float64 {
    return static_cast<viua::float64>(number);
}

auto Integer::operator + (const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{ new Integer(number + that.as_integer()) };
}
auto Integer::operator - (const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{ new Integer(number - that.as_integer()) };
}
auto Integer::operator * (const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{ new Integer(number * that.as_integer()) };
}
auto Integer::operator / (const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{ new Integer(number / that.as_integer()) };
}

auto Integer::operator < (const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{ new Boolean(number < that.as_integer()) };
}
auto Integer::operator <= (const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{ new Boolean(number <= that.as_integer()) };
}
auto Integer::operator > (const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{ new Boolean(number > that.as_integer()) };
}
auto Integer::operator >= (const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{ new Boolean(number >= that.as_integer()) };
}
auto Integer::operator == (const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{ new Boolean(number == that.as_integer()) };
}
