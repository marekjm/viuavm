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

Prototype* Prototype::attach(const string& function_name, const string& method_name) {
    methods[method_name] = function_name;
    return this;
}

bool Prototype::accepts(const string& method_name) const {
    return methods.count(method_name);
}

string Prototype::resolvesTo(const string& method_name) const {
    return methods.at(method_name);
}
