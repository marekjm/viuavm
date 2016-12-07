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
#include <viua/types/closure.h>
#include <viua/types/type.h>
using namespace std;


viua::types::Closure::Closure(const string& name, unique_ptr<viua::kernel::RegisterSet> rs): local_register_set(std::move(rs)), function_name(name) {
}

viua::types::Closure::~Closure() {
}


string viua::types::Closure::type() const {
    return "Closure";
}

string viua::types::Closure::str() const {
    ostringstream oss;
    oss << "Closure: " << function_name;
    return oss.str();
}

string viua::types::Closure::repr() const {
    return str();
}

bool viua::types::Closure::boolean() const {
    return true;
}

unique_ptr<viua::types::Type> viua::types::Closure::copy() const {
    return unique_ptr<viua::types::Closure>{new viua::types::Closure(function_name, std::move(local_register_set->copy()))};
}


string viua::types::Closure::name() const {
    return function_name;
}

viua::kernel::RegisterSet* viua::types::Closure::rs() const {
    return local_register_set.get();
}

void viua::types::Closure::set(unsigned index, unique_ptr<viua::types::Type> object) {
    local_register_set->set(index, std::move(object));
}
