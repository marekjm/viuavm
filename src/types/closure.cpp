#include <string>
#include <sstream>
#include "closure.h"
#include "type.h"
using namespace std;


Closure::Closure(): regset(0), function_name("") {
}

Closure::~Closure() {
    delete regset;
}


string Closure::type() const {
    return "Closure";
}

string Closure::str() const {
    ostringstream oss;
    oss << "Closure: " << function_name;
    return oss.str();
}

string Closure::repr() const {
    return str();
}

bool Closure::boolean() const {
    return true;
}

Type* Closure::copy() const {
    Closure* clsr = new Closure();
    clsr->function_name = function_name;
    // FIXME: for the above one, copy ctor would be nice
    clsr->regset = regset->copy();
    return clsr;
}
