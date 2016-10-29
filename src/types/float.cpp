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

#include <string>
#include <sstream>
#include <viua/types/float.h>
using namespace std;

string Float::type() const {
    return "Float";
}
string Float::str() const {
    std::ostringstream s;
    // std::fixed because 1.0 will yield '1' and not '1.0' when stringified
    s << std::fixed << number;
    return s.str();
}
bool Float::boolean() const {
    return (number != 0);
}

auto Float::value() -> decltype(number)& {
    return number;
}

Type* Float::copy() const {
    return new Float(number);
}

int8_t Float::as_int8() const {
    return static_cast<int8_t>(number);
}
int16_t Float::as_int16() const {
    return static_cast<int16_t>(number);
}
int32_t Float::as_int32() const {
    return static_cast<int32_t>(number);
}
int64_t Float::as_int64() const {
    return static_cast<int64_t>(number);
}

uint8_t Float::as_uint8() const {
    return static_cast<uint8_t>(number);
}
uint16_t Float::as_uint16() const {
    return static_cast<uint16_t>(number);
}
uint32_t Float::as_uint32() const {
    return static_cast<uint32_t>(number);
}
uint64_t Float::as_uint64() const {
    return static_cast<uint64_t>(number);
}

viua::float32 Float::as_float32() const {
    return static_cast<viua::float32>(number);
}
viua::float64 Float::as_float64() const {
    return static_cast<viua::float64>(number);
}

Float::Float(decltype(number) n): number(n) {}
