#include <cmath>
#include <iostream>
#include <viua/types/type.h>
#include <viua/types/float.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


Type* math_sqrt(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected float as first argument");
    }
    if (frame->args->at(0)->type() != "Float") {
        throw new Exception("invalid type of parameter 0: expected Float");
    }

    Float* flt = static_cast<Float*>(frame->args->get(0));
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
