#include <iostream>
#include <fstream>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void io_getline(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    string line;
    getline(cin, line);
    frame->regset->set(0, new String(line));
}


void io_readtext(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    string path = frame->args->get(0)->str();
    ifstream in(path);

    ostringstream oss;
    string line;
    while (getline(in, line)) {
        oss << line << '\n';
    }

    frame->regset->set(0, new String(oss.str()));
}

const ForeignFunctionSpec functions[] = {
    { "std::io::getline", &io_getline },
    { "std::io::readtext", &io_readtext },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
