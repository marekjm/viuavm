#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/types/string.h"
#include "../src/types/vector.h"
#include "../src/include/module.h"
using namespace std;

Object* io_getline(Frame* frame, RegisterSet*, RegisterSet*) {
    string s;
    getline(cin, s);
    frame->regset->set(0, new String(s));
    return 0;
}

Object* io_read(Frame* frame, RegisterSet*, RegisterSet*) {
    ifstream in(dynamic_cast<String*>(frame->args->get(0))->value());
    ostringstream oss;
    string line;

    while (getline(in, line)) { oss << line << '\n'; }
    frame->regset->set(0, new String(oss.str()));
    return 0;
}

Object* io_write(Frame* frame, RegisterSet*, RegisterSet*) {
    ofstream out(dynamic_cast<String*>(frame->args->get(0))->value());
    out << dynamic_cast<String*>(frame->args->get(1))->value();
    out.close();
    return 0;
}

/* Object* io_readlines(Frame* frame, RegisterSet*, RegisterSet*) { */
/*     ifstream in(static_cast<String*>(frame->args->get(0))->value()); */
/*     string line; */

/*     Vector* vec = new Vector(); */

/*     while (getline(cin, line)) { vec->push(new String(line)); } */
/*     frame->regset->set(0, vec); */
/*     return 0; */
/* } */


const char* function_names[] = {
    "getline",
    "read",
    "write",
    /* "readlines", */
    NULL,
};
const ExternalFunction* function_pointers[] = {
    &io_getline,
    &io_read,
    &io_write,
    /* &io_readlines, */
    NULL,
};


extern "C" const char** exports_names() {
    return function_names;
}
extern "C" const ExternalFunction** exports_pointers() {
    return function_pointers;
}
