#include <unistd.h>
#include <iostream>
#include <viua/types/type.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void kitchensink_sleep(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    sleep(static_cast<unsigned int>(static_cast<Integer*>(frame->args->at(0))->as_integer()));
}

const ForeignFunctionSpec functions[] = {
    { "std::kitchensink::sleep/1", &kitchensink_sleep },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
