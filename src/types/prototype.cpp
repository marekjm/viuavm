#include <string>
#include <sstream>
#include <viua/types/prototype.h>
using namespace std;


string Prototype::type() const {
    return "Prototype";
}
bool Prototype::boolean() const {
    return true;
}

Type* Prototype::copy() const {
    Prototype* cp = new Prototype(type_name);
    return cp;
}


string Prototype::getTypeName() const {
    return type_name;
}
