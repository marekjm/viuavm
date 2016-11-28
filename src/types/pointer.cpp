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
#include <algorithm>
#include <viua/types/type.h>
#include <viua/types/boolean.h>
#include <viua/types/pointer.h>
#include <viua/types/exception.h>
using namespace std;


void viua::types::Pointer::attach() {
    points_to->pointers.push_back(this);
    valid = true;
}
void viua::types::Pointer::detach() {
    if (valid) {
        points_to->pointers.erase(std::find(points_to->pointers.begin(), points_to->pointers.end(), this));
    }
    valid = false;
}

void viua::types::Pointer::invalidate(viua::types::Type* t) {
    if (t == points_to) {
        valid = false;
    }
}
bool viua::types::Pointer::expired() {
    return !valid;
}
void viua::types::Pointer::reset(viua::types::Type* t) {
    detach();
    points_to = t;
    attach();
}
viua::types::Type* viua::types::Pointer::to() {
    if (not valid) {
        throw new viua::types::Exception("expired pointer exception");
    }
    return points_to;
}

string viua::types::Pointer::type() const {
    return ((valid ? points_to->type() : "Expired") + "Pointer");
}

bool viua::types::Pointer::boolean() const {
    return valid;
}

string viua::types::Pointer::str() const {
    return type();
}

unique_ptr<viua::types::Type> viua::types::Pointer::copy() const {
    if (not valid) {
        return unique_ptr<viua::types::Type>{new viua::types::Pointer()};
    }
    return unique_ptr<viua::types::Type>{new viua::types::Pointer(points_to)};
}


void viua::types::Pointer::expired(Frame* frm, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    frm->regset->set(0, unique_ptr<viua::types::Type>{new viua::types::Boolean(expired())});
}


viua::types::Pointer::Pointer(): points_to(nullptr), valid(false) {}
viua::types::Pointer::Pointer(viua::types::Type* t): points_to(t), valid(true) {
    attach();
}
viua::types::Pointer::~Pointer() {
    detach();
}
