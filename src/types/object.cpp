/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#include <iostream>
#include <string>
#include <sstream>
#include <viua/types/object.h>
#include <viua/types/exception.h>
#include <viua/kernel/frame.h>
using namespace std;


string viua::types::Object::type() const {
    return type_name;
}
bool viua::types::Object::boolean() const {
    return true;
}

string viua::types::Object::str() const {
    ostringstream oss;

    oss << type_name << '#';
    oss << '{';
    const auto limit = attributes.size();
    std::remove_const<decltype(limit)>::type i = 0;
    for (const auto& each : attributes) {
        oss << each.first << ": " << each.second->repr();
        if (++i < limit) {
            oss << ", ";
        }
    }
    oss << '}';

    return oss.str();
}

unique_ptr<viua::types::Type> viua::types::Object::copy() const {
    unique_ptr<viua::types::Object> cp {new viua::types::Object(type_name)};
    for (const auto& each : attributes) {
        cp->set(each.first, each.second->copy().release());
    }
    return std::move(cp);
}

void viua::types::Object::set(const string& name, viua::types::Type* object) {
    attributes[name] = unique_ptr<viua::types::Type>(object);
}

void viua::types::Object::insert(const string& key, viua::types::Type* value) {
    set(key, value);
}
viua::types::Type* viua::types::Object::remove(const string& key) {
    if (not attributes.count(key)) {
        ostringstream oss;
        oss << "attribute not found: " << key;
        throw new viua::types::Exception(oss.str());
    }
    auto o = std::move(attributes.at(key));
    attributes.erase(key);
    return o.release();
}


viua::types::Object::Object(const std::string& tn): type_name(tn) {}
viua::types::Object::~Object() {
}
