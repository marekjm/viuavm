#include <string>
#include <vector>
#include <sstream>
#include "object.h"
#include "vector.h"
using namespace std;


Object* Vector::insert(int pos, Object* object) {
    // FIXME: implement
    return this;
}

Object* Vector::push(Object* object) {
    internal_object.push_back(object);
    return this;
}

Object* Vector::pop(int index) {
    // FIXME: implement
    return this;
}

Object* Vector::at(int index) {
    return internal_object[index];
}

int Vector::len() {
    // FIXME: should return unsigned
    return (int)internal_object.size();
}
