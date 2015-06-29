#include <iostream>
#include "../../../src/types/type.h"
#include "../../../src/cpu/frame.h"
#include "../../../src/cpu/registerset.h"
#include "../../../src/include/module.h"
using namespace std;


Type* hello(Frame*, RegisterSet*, RegisterSet*) {
    cout << "Hello World!" << endl;
    return 0;
}


const ExternalFunctionSpec functions[] = {
    { "print_hello", &hello },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
