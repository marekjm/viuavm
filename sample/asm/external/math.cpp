#include <cmath>
#include <iostream>
#include <viua/types/type.h>
#include <viua/types/float.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


Type* math_sqrt(Frame* frame, RegisterSet*, RegisterSet*) {
    Float* flt = dynamic_cast<Float*>(frame->args->get(0));
    float square_root = sqrt(flt->value());
    frame->regset->set(0, new Float(square_root));
    return 0;
}


const ExternalFunctionSpec functions[] = {
    { "math::sqrt", &math_sqrt },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
