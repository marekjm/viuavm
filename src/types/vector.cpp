#include <string>
#include <vector>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/vector.h>
#include <viua/exceptions.h>
using namespace std;


Type* Vector::insert(long int index, Type* object) {
    if (index < 0) { index = (internal_object.size()+index); }
    if ((index < 0) or (index >= static_cast<long int>(internal_object.size()) and internal_object.size() != 0)) {
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
    // FIXME: allow popping from arbitrary indexes
    Type* ptr = internal_object.back();
    internal_object.pop_back();
    return ptr;
}

Type* Vector::at(long int index) {
    if (index < 0) { index = (internal_object.size()+index); }
    if ((index < 0) or (index >= (int)internal_object.size())) {
        throw new OutOfRangeException("vector index out of range");
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
