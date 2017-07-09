/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <viua/types/bits.h>
using namespace std;
using namespace viua::types;

const string viua::types::Bits::type_name = "Bits";

string viua::types::Bits::type() const { return type_name; }

string viua::types::Bits::str() const {
    ostringstream oss;

    for (size_type i = underlying_array.size(); i; --i) {
        oss << underlying_array.at(i - 1);
        if (((i - 1) % 4 == 0) and i != 1) {
            oss << ':';
        }
    }

    return oss.str();
}

bool viua::types::Bits::boolean() const { return false; }

unique_ptr<viua::types::Value> viua::types::Bits::copy() const {
    return unique_ptr<viua::types::Value>{new Bits(underlying_array)};
}


auto viua::types::Bits::at(size_type i) const -> bool { return underlying_array.at(i); }

auto viua::types::Bits::set(size_type i, const bool value) -> bool {
    bool was = at(i);
    underlying_array.at(i) = value;
    return was;
}

viua::types::Bits::Bits(const vector<bool>& bs) {
    underlying_array.reserve(bs.size());
    for (const auto each : bs) {
        underlying_array.push_back(each);
    }
}

viua::types::Bits::Bits(size_type i) {
    underlying_array.reserve(i);
    for (; i; --i) {
        underlying_array.push_back(false);
    }
}
