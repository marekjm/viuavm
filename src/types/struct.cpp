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
#include <viua/support/string.h>
#include <viua/types/struct.h>
using namespace std;

const string viua::types::Struct::type_name = "Struct";

string viua::types::Struct::type() const {
    return "Struct";
}

bool viua::types::Struct::boolean() const {
    return (not attributes.empty());
}

string viua::types::Struct::str() const {
    ostringstream oss;

    oss << '{';

    auto i = attributes.size();
    for (const auto& each : attributes) {
        oss << str::enquote(each.first, '\'') << ": " << each.second->repr();
        if (--i) {
            oss << ", ";
        }
    }

    oss << '}';

    return oss.str();
}

string viua::types::Struct::repr() const {
    return str();
}

vector<string> viua::types::Struct::bases() const {
    return vector<string>{"Value"};
}
vector<string> viua::types::Struct::inheritancechain() const {
    return vector<string>{"Value"};
}

void viua::types::Struct::insert(const string& key, unique_ptr<viua::types::Value> value) {
    attributes[key] = std::move(value);
}

unique_ptr<viua::types::Value> viua::types::Struct::remove(const string& key) {
    unique_ptr<viua::types::Value> value = std::move(attributes.at(key));
    attributes.erase(key);
    return value;
}

vector<string> viua::types::Struct::keys() const {
    vector<string> ks;
    for (const auto& each : attributes) {
        ks.push_back(each.first);
    }
    return ks;
}

unique_ptr<viua::types::Value> viua::types::Struct::copy() const {
    unique_ptr<viua::types::Struct> copied { new Struct() };
    for (const auto& each : attributes) {
        copied->insert(each.first, each.second->copy());
    }
    return copied;
}
