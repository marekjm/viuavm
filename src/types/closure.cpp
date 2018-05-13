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
#include <viua/types/closure.h>
#include <viua/types/value.h>
using namespace std;

const string viua::types::Closure::type_name = "Closure";


viua::types::Closure::Closure(const string& name,
                              unique_ptr<viua::kernel::RegisterSet> rs)
        : viua::types::Function::Function{name}, local_register_set{std::move(rs)} {}

viua::types::Closure::~Closure() {}


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

unique_ptr<viua::types::Value> viua::types::Closure::copy() const {
    return make_unique<Closure>(function_name, local_register_set->copy());
}


string viua::types::Closure::name() const {
    return function_name;
}

viua::kernel::RegisterSet* viua::types::Closure::rs() const {
    return local_register_set.get();
}

auto viua::types::Closure::release() -> viua::kernel::RegisterSet* {
    return local_register_set.release();
}

auto viua::types::Closure::give() -> unique_ptr<viua::kernel::RegisterSet> {
    return std::move(local_register_set);
}

auto viua::types::Closure::empty() const -> bool {
    return (local_register_set == nullptr);
}

void viua::types::Closure::set(viua::internals::types::register_index index,
                               unique_ptr<viua::types::Value> object) {
    local_register_set->set(index, std::move(object));
}
