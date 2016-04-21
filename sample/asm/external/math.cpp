#include <cmath>
#include <iostream>
#include <viua/types/type.h>
#include <viua/types/float.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void math_sqrt(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    if (frame->args->at(0) == nullptr) {
        throw new Exception("expected float as first argument");
    }
    if (frame->args->at(0)->type() != "Float") {
        throw new Exception("invalid type of parameter 0: expected Float");
    }

    float square_root = sqrt(static_cast<Float*>(frame->args->at(0))->value());
    frame->regset->set(0, new Float(square_root));
}


const ForeignFunctionSpec functions[] = {
    { "math::sqrt", &math_sqrt },
    { nullptr, nullptr },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
