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

const std::string viua::types::Struct::type_name = "Struct";

std::string viua::types::Struct::type() const {
    return "Struct";
}

bool viua::types::Struct::boolean() const {
    return (not attributes.empty());
}

std::string viua::types::Struct::str() const {
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

std::string viua::types::Struct::repr() const {
    return str();
}

void viua::types::Struct::insert(const std::string& key,
                                 std::unique_ptr<viua::types::Value> value) {
    attributes[key] = std::move(value);
}

std::unique_ptr<viua::types::Value> viua::types::Struct::remove(
    const std::string& key) {
    std::unique_ptr<viua::types::Value> value = std::move(attributes.at(key));
    attributes.erase(key);
    return value;
}

vector<std::string> viua::types::Struct::keys() const {
    vector<std::string> ks;
    for (const auto& each : attributes) {
        ks.push_back(each.first);
    }
    return ks;
}

std::unique_ptr<viua::types::Value> viua::types::Struct::copy() const {
    auto copied = make_unique<Struct>();
    for (const auto& each : attributes) {
        copied->insert(each.first, each.second->copy());
    }
    return copied;
}
