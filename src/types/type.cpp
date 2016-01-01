#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <viua/types/type.h>
#include <viua/types/pointer.h>
#include <viua/types/exception.h>
using namespace std;


Pointer* Type::pointer() {
    return new Pointer(this);
}

Type::~Type() {
    for (auto p : pointers) {
        p->invalidate(this);
    }
}
