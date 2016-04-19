#include <cmath>
#include <iostream>
#include <viua/types/type.h>
#include <viua/types/float.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void throwing_oh_noes(Frame*, RegisterSet*, RegisterSet*) {
    throw new Exception("OH NOES!");
}


const ExternalFunctionSpec functions[] = {
    { "throwing::oh_noes", &throwing_oh_noes },
    { nullptr, nullptr },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
