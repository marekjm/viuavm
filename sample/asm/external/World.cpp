#include <iostream>
#include <viua/types/type.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void hello(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*) {
    cout << "Hello World!" << endl;
}


const ExternalFunctionSpec functions[] = {
    { "World::print_hello", &hello },
    { nullptr, nullptr },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
