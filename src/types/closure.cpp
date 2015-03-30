#include <string>
#include <sstream>
#include "object.h"
#include "closure.h"
using namespace std;


Closure::Closure():
    arguments(0), argreferences(0), arguments_size(0),
    registers(0), references(0), registers_size(0),
    function_name("") {
}

Closure::~Closure() {
    for (int i = 0; i < registers_size; ++i) {
        if (registers[i] != 0 and !references[i]) { delete registers[i]; }
    }
    if (registers_size) {
        delete[] registers;
        delete[] references;
    }
}


string Closure::type() const {
    return "Closure";
}

string Closure::str() const {
    ostringstream oss;
    oss << "Closure: " << function_name;
    return oss.str();
}

string Closure::repr() const {
    return str();
}

bool Closure::boolean() const {
    return true;
}

Object* Closure::copy() const {
    Closure* clsr = new Closure();
    clsr->function_name = function_name;
    // FIXME: we should copy the registers instead of just pointing to them
    // FIXME: for the above one, copy ctor would be nice
    clsr->registers = registers;
    clsr->references = references;
    clsr->registers_size = registers_size;
    return clsr;
}
