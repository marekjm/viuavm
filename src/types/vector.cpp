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
#include <vector>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/vector.h>
#include <viua/exceptions.h>
using namespace std;


Type* Vector::insert(long int index, Type* object) {
    if (index < 0) { index = (internal_object.size()+index); }
    if ((index < 0) or (index > (static_cast<long int>(internal_object.size())+1) and index > 0)) {
        throw new OutOfRangeException("vector index out of range");
    }
    vector<Type*>::iterator it = (internal_object.begin()+index);
    internal_object.insert(it, object);
    return object;
}

Type* Vector::push(Type* object) {
    internal_object.push_back(object);
    return object;
}

Type* Vector::pop(long int index) {
    if (index < 0) { index = (internal_object.size()+index); }
    if ((index < 0) or (index >= static_cast<long int>(internal_object.size()) and index > 0)) {
        throw new OutOfRangeException("vector index out of range");
    }
    Type* object = internal_object[index];
    vector<Type*>::iterator it = (internal_object.begin()+index);
    internal_object.erase(it);
    return object;
}

Type* Vector::at(long int index) {
    if (index < 0) { index = (internal_object.size()+index); }
    // FIXME: convert signed indexes on input to unsigned indexes for internal usage
    // to avoid the cast
    if ((index < 0) or (index >= static_cast<long int>(internal_object.size()))) {
        throw new OutOfRangeException("vector index out of range");
    }
    // FIXME: returned value is a reference, but docs say it's a copy
    // FIXME: this is a *serious* bug, if user code `delete`'s the object
    // VM will segfault
    return internal_object[index];
}

int Vector::len() {
    // FIXME: should return unsigned
    // FIXME: VM does not have unsigned integer type so return value has
    // to be converted to signed integer
    return static_cast<int>(internal_object.size());
}

string Vector::str() const {
    ostringstream oss;
    oss << "[";
    for (unsigned i = 0; i < internal_object.size(); ++i) {
        oss << internal_object[i]->repr() << (i < internal_object.size()-1 ? ", " : "");
    }
    oss << "]";
    return oss.str();
}
