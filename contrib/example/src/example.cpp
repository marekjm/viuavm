#include <time.h>
#include <iostream>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


namespace example {
    void hello_world(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*) {
        cout << "Hello World!" << endl;
    }

    void hello(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
        if (frame->args->size() == 0) {
            throw new Exception("example::hello(String who): expected a parameter, got none");
        }
        cout << "Hello " << frame->args->at(0)->str() << "!" << endl;
    }

    void what_time_is_it(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
        frame->regset->set(0, new Integer(time(nullptr)));
    }
}


// A list describing exported functions.
const ForeignFunctionSpec functions[] = {
    { "example::hello_world", example::hello_world },
    { "example::hello", example::hello },
    { "example::what_time_is_it", example::what_time_is_it },
    { nullptr, nullptr }
};

// A function the VM will call to extract the list of functions and
// their addresses.
extern "C" const ForeignFunctionSpec* exports() { return functions; }
