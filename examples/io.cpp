#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/include/module.h>
using namespace std;

void io_getline(Frame* frame, RegisterSet*, RegisterSet*) {
    string s;
    getline(cin, s);
    frame->regset->set(0, new String(s));
}

void io_read(Frame* frame, RegisterSet*, RegisterSet*) {
    ifstream in(dynamic_cast<String*>(frame->args->get(0))->value());
    ostringstream oss;
    string line;

    while (getline(in, line)) { oss << line << '\n'; }
    frame->regset->set(0, new String(oss.str()));
}

void io_write(Frame* frame, RegisterSet*, RegisterSet*) {
    ofstream out(dynamic_cast<String*>(frame->args->get(0))->value());
    out << dynamic_cast<String*>(frame->args->get(1))->value();
    out.close();
}


const char* function_names[] = {
    "io::getline",
    "io::read",
    "io::write",
    NULL,
};
const ExternalFunction* function_pointers[] = {
    &io_getline,
    &io_read,
    &io_write,
    NULL,
};


extern "C" const char** exports_names() {
    return function_names;
}
extern "C" const ExternalFunction** exports_pointers() {
    return function_pointers;
}
