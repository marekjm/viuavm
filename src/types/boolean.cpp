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

#include <viua/types/boolean.h>
using namespace std;

const string viua::types::Boolean::type_name = "Boolean";

string viua::types::Boolean::type() const {
    return "Boolean";
}

string viua::types::Boolean::str() const {
    return (b ? "true" : "false");
}

bool viua::types::Boolean::boolean() const {
    return b;
}

bool& viua::types::Boolean::value() {
    return b;
}

vector<string> viua::types::Boolean::bases() const {
    return vector<string>{"Number"};
}
vector<string> viua::types::Boolean::inheritancechain() const {
    return vector<string>{"Number", "Value"};
}

unique_ptr<viua::types::Value> viua::types::Boolean::copy() const {
    return make_unique<Boolean>(b);
}

viua::types::Boolean::Boolean(bool v) : b(v) {}
