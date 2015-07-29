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
