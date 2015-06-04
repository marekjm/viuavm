#include <string>
#include <sstream>
#include "type.h"
#include "function.h"
using namespace std;


Function::Function():
    arguments(0), argreferences(0), arguments_size(0),
    function_name("") {
}

Function::~Function() {
}


string Function::type() const {
    return "Function";
}

string Function::str() const {
    ostringstream oss;
    oss << "Function: " << function_name;
    return oss.str();
}

string Function::repr() const {
    return str();
}

bool Function::boolean() const {
    return true;
}

Type* Function::copy() const {
    Function* fn = new Function();
    fn->function_name = function_name;
    return fn;
}
