#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <viua/support/string.h>
#include <viua/types/type.h>
#include <viua/types/vector.h>
#include <viua/types/string.h>
#include <viua/exceptions.h>
using namespace std;


Integer* String::size() {
    /** Return size of the string.
     */
    return new Integer(int(svalue.size()));
}

String* String::sub(int b, int e) {
    /** Return substring extracted from this object.
     */
    return new String(str::sub(svalue, b, e));
}

String* String::add(String* s) {
    /** Append string to this string.
     */
    svalue += s->value();
    return this;
}

String* String::join(Vector* v) {
    /** Use this string to join objects in vector.
     */
    string s = "";
    int vector_len = v->len();
    for (int i = 0; i < vector_len; ++i) {
        s += v->at(i)->str();
        if (i < (vector_len-1)) {
            s += svalue;
        }
    }
    return new String(s);
}

// foreign methods
Type* String::stringify(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() == 0) {
        throw new Exception("expected 1 parameter but got 0");
    }
    frame->regset->set(0, new String(frame->args->at(0)->str()));
    return 0;
}

Type* String::represent(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() == 0) {
        throw new Exception("expected 1 parameter but got 0");
    }
    frame->regset->set(0, new String(frame->args->at(0)->repr()));
    return 0;
}
