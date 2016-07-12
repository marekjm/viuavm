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


void Pointer::attach() {
    points_to->pointers.push_back(this);
    valid = true;
}
void Pointer::detach() {
    if (valid) {
        points_to->pointers.erase(std::find(points_to->pointers.begin(), points_to->pointers.end(), this));
    }
    valid = false;
}

void Pointer::invalidate(Type* t) {
    if (t == points_to) {
        valid = false;
    }
}
bool Pointer::expired() {
    return !valid;
}
void Pointer::reset(Type* t) {
    detach();
    points_to = t;
    attach();
}
Type* Pointer::to() {
    if (not valid) {
        throw new Exception("expired pointer exception");
    }
    return points_to;
}

string Pointer::type() const {
    return ((valid ? points_to->type() : "Expired") + "Pointer");
}

bool Pointer::boolean() const {
    return valid;
}

string Pointer::str() const {
    return type();
}

Type* Pointer::copy() const {
    if (not valid) {
        return new Pointer();
    } else {
        return new Pointer(points_to);
    }
}


void Pointer::expired(Frame* frm, RegisterSet*, RegisterSet*, Process*, CPU*) {
    frm->regset->set(0, new Boolean(expired()));
}


Pointer::Pointer(): points_to(nullptr), valid(false) {}
Pointer::Pointer(Type* t): points_to(t), valid(true) {
    attach();
}
Pointer::~Pointer() {
    detach();
}
