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
    long offset = 0;

    // FIXME: REFACTORING: move bounds-checking to a separate function
    if (index > 0 and static_cast<decltype(internal_object)::size_type>(index) > internal_object.size()) {
        throw new OutOfRangeException("positive vector index out of range");
    } else if (index < 0 and static_cast<decltype(internal_object)::size_type>(-index) > internal_object.size()) {
        throw new OutOfRangeException("negative vector index out of range");
    }
    if (index < 0) {
        offset = (static_cast<decltype(index)>(internal_object.size()) + index);
    } else {
        offset = index;
    }

    vector<Type*>::iterator it = (internal_object.begin()+offset);
    internal_object.insert(it, object);
    return object;
}

Type* Vector::push(Type* object) {
    internal_object.push_back(object);
    return object;
}

Type* Vector::pop(long int index) {
    long offset = 0;

    // FIXME: REFACTORING: move bounds-checking to a separate function
    if (index > 0 and static_cast<decltype(internal_object)::size_type>(index) >= internal_object.size()) {
        throw new OutOfRangeException("positive vector index out of range");
    } else if (index < 0 and static_cast<decltype(internal_object)::size_type>(-index) > internal_object.size()) {
        throw new OutOfRangeException("negative vector index out of range");
    }

    if (index < 0) {
        offset = (static_cast<decltype(index)>(internal_object.size()) + index);
    } else {
        offset = index;
    }

    vector<Type*>::iterator it = (internal_object.begin()+offset);
    Type *object = *it;
    internal_object.erase(it);
    return object;
}

Type* Vector::at(long int index) {
    long offset = 0;

    // FIXME: REFACTORING: move bounds-checking to a separate function
    if (index > 0 and static_cast<decltype(internal_object)::size_type>(index) >= internal_object.size()) {
        throw new OutOfRangeException("positive vector index out of range");
    } else if (index < 0 and static_cast<decltype(internal_object)::size_type>(-index) > internal_object.size()) {
        throw new OutOfRangeException("negative vector index out of range");
    }

    if (index < 0) {
        offset = (static_cast<decltype(index)>(internal_object.size()) + index);
    } else {
        offset = index;
    }

    vector<Type*>::iterator it = (internal_object.begin()+offset);
    return *it;
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
