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

#include <sstream>
#include <viua/types/reference.h>
using namespace std;


viua::types::Type* viua::types::Reference::pointsTo() const {
    return *pointer;
}

void viua::types::Reference::rebind(viua::types::Type* ptr) {
    if (*pointer) {
        delete (*pointer);
    }
    (*pointer) = ptr;
}

void viua::types::Reference::rebind(unique_ptr<viua::types::Type> ptr) {
    if (*pointer) {
        delete (*pointer);
    }
    (*pointer) = ptr.release();
}


string viua::types::Reference::type() const {
    return (*pointer)->type();
}

string viua::types::Reference::str() const {
    return (*pointer)->str();
}
string viua::types::Reference::repr() const {
    return (*pointer)->repr();
}
bool viua::types::Reference::boolean() const {
    return (*pointer)->boolean();
}

vector<string> viua::types::Reference::bases() const {
    return vector<string>({});
}
vector<string> viua::types::Reference::inheritancechain() const {
    return vector<string>({});
}

unique_ptr<viua::types::Type> viua::types::Reference::copy() const {
    ++(*counter);
    return unique_ptr<viua::types::Type>{new viua::types::Reference(pointer, counter)};
}

viua::types::Reference::Reference(viua::types::Type *ptr): pointer(new viua::types::Type*(ptr)), counter(new uint64_t(1)) {
}
viua::types::Reference::~Reference() {
    /** Copies of the reference may be freely spawned and destroyed, but
     *  the internal object *MUST* be preserved until its refcount reaches zero.
     */
    if ((--(*counter)) == 0) {
        cout << hex;
        delete *pointer;
        delete pointer;
        delete counter;
    }
}
