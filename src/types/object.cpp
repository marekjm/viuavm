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

void Object::set(const string& name, Type* object) {
    // prevent memory leaks during key overwriting
    if (attributes.count(name)) {
        delete attributes.at(name);
        attributes.erase(name);
    }
    attributes[name] = object;
}

void Object::insert(const string& key, Type* value) {
    set(key, value);
}
Type* Object::remove(const string& key) {
    Type* o = attributes.at(key);
    attributes.erase(key);
    return o;
}


Object::Object(const std::string& tn): type_name(tn) {}
Object::~Object() {
    auto kv_pair = attributes.begin();
    while (kv_pair != attributes.end()) {
        string key = kv_pair->first;
        Type* value = kv_pair->second;

        ++kv_pair;

        attributes.erase(key);
        delete value;
    }
}
