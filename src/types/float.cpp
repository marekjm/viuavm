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

#include <sstream>
#include <string>
#include <viua/types/boolean.h>
#include <viua/types/float.h>
using namespace std;
using namespace viua::types;

const string viua::types::Float::type_name = "Float";


string Float::type() const { return "Float"; }
string Float::str() const { return to_string(number); }
bool Float::boolean() const { return (number != 0); }

auto Float::value() -> decltype(number) & { return number; }

unique_ptr<viua::types::Value> Float::copy() const {
    return unique_ptr<viua::types::Value>{new Float(number)};
}

auto Float::as_integer() const -> int64_t { return static_cast<int64_t>(number); }

auto Float::as_float() const -> viua::float64 { return number; }

auto Float::operator+(const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{new Float(number + that.as_float())};
}
auto Float::operator-(const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{new Float(number - that.as_float())};
}
auto Float::operator*(const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{new Float(number * that.as_float())};
}
auto Float::operator/(const numeric::Number& that) const -> unique_ptr<numeric::Number> {
    return unique_ptr<numeric::Number>{new Float(number / that.as_float())};
}

auto Float::operator<(const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{new Boolean(number < that.as_float())};
}
auto Float::operator<=(const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{new Boolean(number <= that.as_float())};
}
auto Float::operator>(const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{new Boolean(number > that.as_float())};
}
auto Float::operator>=(const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{new Boolean(number >= that.as_float())};
}
auto Float::operator==(const numeric::Number& that) const -> unique_ptr<Boolean> {
    return unique_ptr<Boolean>{new Boolean(number == that.as_float())};
}

Float::Float(decltype(number) n) : number(n) {}
