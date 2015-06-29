#include <string>
#include <vector>
#include "../types/type.h"
#include "../types/string.h"
#include "../types/vector.h"
#include "../types/exception.h"
#include "../cpu/frame.h"
#include "../cpu/registerset.h"
#include "../include/module.h"
using namespace std;


Type* string_string(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->str()));
    return 0;
}

Type* string_repr(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->repr()));
    return 0;
}

Type* string_stringify(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(str::enquote(frame->args->get(0)->str())));
    return 0;
}


const ExternalFunctionSpec functions[] = {
    { "string", &string_string },
    { "repr", &string_repr },
    { "stringify", &string_stringify },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
