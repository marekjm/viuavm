#include <iostream>
#include <viua/types/type.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


Type* io_getline(Frame* frame, RegisterSet*, RegisterSet*) {
    string line;
    getline(cin, line);
    frame->regset->set(0, new String(line));
    return 0;
}

const ExternalFunctionSpec functions[] = {
    { "std::io::getline", &io_getline },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
