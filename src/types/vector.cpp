#include <string>
#include <vector>
#include <sstream>
#include "object.h"
#include "vector.h"
#include "../exceptions.h"
using namespace std;


Object* Vector::insert(int index, Object* object) {
    if (index < 0) { index = (internal_object.size()+index); }
    if ((index < 0) or (index >= (int)internal_object.size() and internal_object.size() != 0)) {
        throw OutOfRangeException("vector index out of range");
    }
    vector<Object*>::iterator it = (internal_object.begin()+index);
    internal_object.insert(it, object);
    return object;
}

Object* Vector::push(Object* object) {
    internal_object.push_back(object);
    return object;
}

Object* Vector::pop(int index) {
    // FIXME: allow popping from arbitrary indexes
    Object* ptr = internal_object.back();
    internal_object.pop_back();
    return ptr;
}

Object* Vector::at(int index) {
    if (index < 0) { index = (internal_object.size()+index); }
    if ((index < 0) or (index >= (int)internal_object.size())) {
        throw OutOfRangeException("vector index out of range");
    }
    // FIXME: returned value is a reference, but docs say it's a copy
    return internal_object[index];
}

int Vector::len() {
    // FIXME: should return unsigned
    return (int)internal_object.size();
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
