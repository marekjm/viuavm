#include <string>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/function.h>
using namespace std;


Function::Function(): function_name("") {
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


string Function::name() const {
    return function_name;
}
