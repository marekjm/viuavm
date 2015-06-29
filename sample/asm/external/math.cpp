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


const ExternalFunctionSpec functions[] = {
    { "sqrt", &math_sqrt },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
