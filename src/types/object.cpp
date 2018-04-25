/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <memory>
#include <sstream>
#include <string>
#include <viua/kernel/frame.h>
#include <viua/types/exception.h>
#include <viua/types/object.h>
using namespace std;

const string viua::types::Object::type_name = "Object";

string viua::types::Object::type() const {
    return object_type_name;
}
bool viua::types::Object::boolean() const {
    return true;
}

string viua::types::Object::str() const {
    ostringstream oss;

    oss << object_type_name << '#';
    oss << '{';
    const auto limit                           = attributes.size();
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

unique_ptr<viua::types::Value> viua::types::Object::copy() const {
    auto cp = make_unique<viua::types::Object>(object_type_name);
    for (const auto& each : attributes) {
        cp->set(each.first, each.second->copy());
    }
    return std::move(cp);
}

void viua::types::Object::set(const string& name,
                              unique_ptr<viua::types::Value> object) {
    attributes[name] = std::move(object);
}

void viua::types::Object::insert(const string& key,
                                 unique_ptr<viua::types::Value> value) {
    set(key, std::move(value));
}
unique_ptr<viua::types::Value> viua::types::Object::remove(const string& key) {
    if (not attributes.count(key)) {
        ostringstream oss;
        oss << "attribute not found: " << key;
        throw make_unique<viua::types::Exception>(oss.str());
    }
    auto o = std::move(attributes.at(key));
    attributes.erase(key);
    return o;
}

viua::types::Object::Object(const std::string& tn) : object_type_name(tn) {}
viua::types::Object::~Object() {}
