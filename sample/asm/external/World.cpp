#include <iostream>
#include "../../../src/types/object.h"
#include "../../../src/cpu/frame.h"
#include "../../../src/cpu/registerset.h"
#include "../../../src/include/module.h"
using namespace std;


Object* hello(Frame*, RegisterSet*, RegisterSet*) {
    cout << "Hello World!" << endl;
    return 0;
}


const char* function_names[] = {
    "print_hello",
    NULL,
};
const ExternalFunction* function_pointers[] = {
    &hello,
    NULL,
};


extern "C" const char** exports_names() {
    return function_names;
}
extern "C" const ExternalFunction** exports_pointers() {
    return function_pointers;
}
