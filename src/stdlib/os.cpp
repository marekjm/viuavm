#include <cstdlib>
#include <string>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


Type* os_system(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected command to launch (string) as parameter 0");
    }
    string command = frame->args->get(0)->str();
    int ret = system(command.c_str());
    frame->regset->set(0, new Integer(ret));
    return 0;
}


const ExternalFunctionSpec functions[] = {
    { "os::system", &os_system },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
