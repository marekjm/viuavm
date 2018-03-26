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

#include <sstream>
#include <string>
#include <viua/types/prototype.h>
using namespace std;

const string viua::types::Prototype::type_name = "Prototype";

string viua::types::Prototype::type() const { return "viua::types::Prototype"; }
bool viua::types::Prototype::boolean() const { return true; }

vector<string> viua::types::Prototype::bases() const {
    return vector<string>{"Value"};
}
vector<string> viua::types::Prototype::inheritancechain() const {
    return vector<string>{"Value"};
}

string viua::types::Prototype::str() const {
    return ("Prototype for " + prototype_name);
}

unique_ptr<viua::types::Value> viua::types::Prototype::copy() const {
    return make_unique<Prototype>(prototype_name);
}


string viua::types::Prototype::get_type_name() const { return prototype_name; }
vector<string> viua::types::Prototype::get_ancestors() const {
    return ancestors;
}

viua::types::Prototype* viua::types::Prototype::derive(
    const string& base_class_name) {
    ancestors.emplace_back(base_class_name);
    return this;
}

viua::types::Prototype* viua::types::Prototype::attach(
    const string& function_name, const string& method_name) {
    methods[method_name] = function_name;
    return this;
}

bool viua::types::Prototype::accepts(const string& method_name) const {
    return methods.count(method_name);
}

string viua::types::Prototype::resolves_to(const string& method_name) const {
    return methods.at(method_name);
}

viua::types::Prototype::Prototype(const string& tn) : prototype_name(tn) {}

viua::types::Prototype::~Prototype() {}
