#include <iostream>
#include <viua/types/type.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


Type* hello(Frame*, RegisterSet*, RegisterSet*) {
    cout << "Hello World!" << endl;
    return 0;
}


const ExternalFunctionSpec functions[] = {
    { "World::print_hello", &hello },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
