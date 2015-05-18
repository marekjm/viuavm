#include <cmath>
#include <iostream>
#include "../../../src/types/type.h"
#include "../../../src/types/float.h"
#include "../../../src/cpu/frame.h"
#include "../../../src/cpu/registerset.h"
#include "../../../src/include/module.h"
using namespace std;


Type* math_sqrt(Frame* frame, RegisterSet*, RegisterSet*) {
    Float* flt = dynamic_cast<Float*>(frame->args->get(0));
    float square_root = sqrt(flt->value());
    frame->regset->set(0, new Float(square_root));
    return 0;
}


const char* function_names[] = {
    "sqrt",
    NULL,
};
const ExternalFunction* function_pointers[] = {
    &math_sqrt,
    NULL,
};


extern "C" const char** exports_names() {
    return function_names;
}
extern "C" const ExternalFunction** exports_pointers() {
    return function_pointers;
}
