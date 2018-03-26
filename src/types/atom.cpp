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

#include <viua/support/string.h>
#include <viua/types/atom.h>
using namespace std;

const string viua::types::Atom::type_name = "viua::types::Atom";

vector<string> viua::types::Atom::bases() const {
    return {"Value"};
}

vector<string> viua::types::Atom::inheritancechain() const {
    return {"Value"};
}

string viua::types::Atom::type() const {
    return "viua::types::Atom";
}

bool viua::types::Atom::boolean() const {
    return true;
}

string viua::types::Atom::str() const {
    return str::enquote(value, '\'');
}

string viua::types::Atom::repr() const {
    return str();
}

viua::types::Atom::operator string() const {
    return value;
}

unique_ptr<viua::types::Value> viua::types::Atom::copy() const {
    return make_unique<Atom>(value);
}

auto viua::types::Atom::operator==(const Atom& that) const -> bool {
    return (value == that.value);
}

viua::types::Atom::Atom(string s) : value(s) {}
