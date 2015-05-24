#include "../types/type.h"
#include "../types/string.h"
#include "../types/exception.h"
#include "../cpu/frame.h"
#include "../cpu/registerset.h"
#include "../include/module.h"
using namespace std;


Type* typeof(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->type()));
    return 0;
}


const char* function_names[] = {
    "typeof",
    NULL,
};
const ExternalFunction* function_pointers[] = {
    &typeof,
    NULL,
};


extern "C" const char** exports_names() {
    return function_names;
}
extern "C" const ExternalFunction** exports_pointers() {
    return function_pointers;
}
