#include <iostream>
#include <string>
#include <sstream>
#include <viua/types/object.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
using namespace std;


string Object::type() const {
    return type_name;
}
bool Object::boolean() const {
    return true;
}

Type* Object::copy() const {
    Object* cp = new Object(type_name);
    return cp;
}

Type* Object::set(Frame*, RegisterSet*, RegisterSet*) {
    cout << "Object::set()" << endl;
    return 0;
}
Type* Object::get(Frame*, RegisterSet*, RegisterSet*) {
    cout << "Object::get()" << endl;
    return 0;
}


Object::Object(const std::string& tn): type_name(tn) {}
Object::~Object() {}
