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

#include <string>
#include <sstream>
#include <viua/types/prototype.h>
using namespace std;


string Prototype::type() const {
    return "Prototype";
}
bool Prototype::boolean() const {
    return true;
}

string Prototype::str() const {
    return ("Prototype for " + type_name);
}

Type* Prototype::copy() const {
    Prototype* cp = new Prototype(type_name);
    return cp;
}


string Prototype::getTypeName() const {
    return type_name;
}
vector<string> Prototype::getAncestors() const {
    return ancestors;
}

Prototype* Prototype::derive(const string& base_class_name) {
    ancestors.emplace_back(base_class_name);
    return this;
}

Prototype* Prototype::attach(const string& function_name, const string& method_name) {
    methods[method_name] = function_name;
    return this;
}

bool Prototype::accepts(const string& method_name) const {
    return methods.count(method_name);
}

string Prototype::resolvesTo(const string& method_name) const {
    return methods.at(method_name);
}
