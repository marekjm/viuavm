#include <string>
#include <vector>

#include <viua/types/type.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void string_string(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->str()));
}

void string_repr(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->repr()));
}

void string_stringify(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(str::enquote(frame->args->get(0)->str())));
}


const ExternalFunctionSpec functions[] = {
    { "string::string", &string_string },
    { "string::repr", &string_repr },
    { "string::stringify", &string_stringify },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
