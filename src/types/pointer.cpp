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

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>
using namespace std;

std::string const viua::types::Pointer::type_name = "Pointer";

void viua::types::Pointer::attach() {
    points_to->pointers.push_back(this);
    valid = true;
}
void viua::types::Pointer::detach() {
    if (valid) {
        points_to->pointers.erase(std::find(
            points_to->pointers.begin(), points_to->pointers.end(), this));
    }
    valid = false;
}

void viua::types::Pointer::invalidate(viua::types::Value* t) {
    if (t == points_to) {
        valid = false;
    }
}
bool viua::types::Pointer::expired() {
    return !valid;
}
auto viua::types::Pointer::authenticate(const viua::process::Process* process)
    -> void {
    /*
     *  Pointers should automatically expire upon crossing process boundaries.
     *  This method should be called before any other method every time the VM
     *  code passes the pointer object to user-process to ensure that Pointer's
     * state is properly accounted for.
     */
    valid = (valid and (process_of_origin == process));
}
void viua::types::Pointer::reset(viua::types::Value* t) {
    detach();
    points_to = t;
    attach();
}
viua::types::Value* viua::types::Pointer::to(const viua::process::Process* p) {
    if (process_of_origin != p) {
        // Dereferencing pointers outside of their original process is illegal.
        throw make_unique<viua::types::Exception>(
            "InvalidDereference: outside of original process");
    }
    if (not valid) {
        throw make_unique<viua::types::Exception>("expired pointer exception");
    }
    return points_to;
}

std::string viua::types::Pointer::type() const {
    return ((valid ? points_to->type() : "Expired") + "Pointer");
}

bool viua::types::Pointer::boolean() const {
    return valid;
}

std::string viua::types::Pointer::str() const {
    return type();
}

std::unique_ptr<viua::types::Value> viua::types::Pointer::copy() const {
    if (not valid) {
        return make_unique<Pointer>(process_of_origin);
    }
    return make_unique<Pointer>(points_to, process_of_origin);
}


viua::types::Pointer::Pointer(const viua::process::Process* poi)
        : points_to(nullptr), valid(false), process_of_origin(poi) {}
viua::types::Pointer::Pointer(viua::types::Value* t,
                              const viua::process::Process* poi)
        : points_to(t), valid(true), process_of_origin(poi) {
    attach();
}
viua::types::Pointer::~Pointer() {
    detach();
}
