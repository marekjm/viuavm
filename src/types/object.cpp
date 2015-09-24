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

void Object::set(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() != 3) {
        ostringstream oss;
        oss << "invalid number of arguments: expected 3 but got " << frame->args->size();
        throw new Exception(oss.str());
    }
    if (frame->args->at(1)->type() != "String") {
        throw new Exception("invalid type of first parameter: expected 'String' but got '" + frame->args->at(1)->type() + "'");
    }

    string name = frame->args->at(1)->str();

    // prevent memory leaks during key overwriting
    if (attributes.count(name)) {
        delete attributes.at(name);
        attributes.erase(name);
    }

    attributes[name] = frame->args->at(2)->copy();
}
void Object::get(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() != 2) {
        ostringstream oss;
        oss << "invalid number of arguments: expected 2 but got " << frame->args->size();
        throw new Exception(oss.str());
    }
    if (frame->args->at(1)->type() != "String") {
        throw new Exception("invalid type of first parameter: expected 'String' but got '" + frame->args->at(1)->type() + "'");
    }

    string name = frame->args->at(1)->str();
    if (attributes.count(name) == 0) {
        throw new Exception("failed attribute lookup: '" + name + "'");
    }

    frame->regset->set(0, attributes[name]->copy());
}


Object::Object(const std::string& tn): type_name(tn) {}
Object::~Object() {}
