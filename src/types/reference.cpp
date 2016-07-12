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


Type* Reference::pointsTo() const {
    return *pointer;
}

void Reference::rebind(Type* ptr) {
    delete (*pointer);
    (*pointer) = ptr;
}


string Reference::type() const {
    return (*pointer)->type();
}

string Reference::str() const {
    return (*pointer)->str();
}
string Reference::repr() const {
    return (*pointer)->repr();
}
bool Reference::boolean() const {
    return (*pointer)->boolean();
}

vector<string> Reference::bases() const {
    return vector<string>({});
}
vector<string> Reference::inheritancechain() const {
    return vector<string>({});
}

Reference* Reference::copy() const {
    ++(*counter);
    return new Reference(pointer, counter);
}

Reference::Reference(Type *ptr): pointer(new Type*(ptr)), counter(new unsigned(1)) {
}
Reference::~Reference() {
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
