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
    if (valid) {
        return type();
    } else {
        return "ExpiredPointer";
    }
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
