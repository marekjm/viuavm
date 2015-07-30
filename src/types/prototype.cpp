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
vector<string> Prototype::getAncestors() const {
    return ancestors;
}

Prototype* Prototype::derive(const string& base_class_name) {
    ancestors.push_back(base_class_name);
    return this;
}
