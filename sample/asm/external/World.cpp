#include <iostream>
#include <viua/types/type.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


Type* hello(Frame*, RegisterSet*, RegisterSet*) {
    cout << "Hello World!" << endl;
    return nullptr;
}


const ExternalFunctionSpec functions[] = {
    { "World::print_hello", &hello },
    { nullptr, nullptr },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
