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

#include <string>
#include <sstream>
#include <viua/types/value.h>
#include <viua/types/function.h>
using namespace std;

const string viua::types::Function::type_name = "Function";


viua::types::Function::Function(const string& name): function_name(name) {
}

viua::types::Function::~Function() {
}


string viua::types::Function::type() const {
    return "Function";
}

string viua::types::Function::str() const {
    ostringstream oss;
    oss << "Function: " << function_name;
    return oss.str();
}

string viua::types::Function::repr() const {
    return str();
}

bool viua::types::Function::boolean() const {
    return true;
}

unique_ptr<viua::types::Value> viua::types::Function::copy() const {
    return unique_ptr<viua::types::Value>{new viua::types::Function(function_name)};
}


string viua::types::Function::name() const {
    return function_name;
}
