#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include "../support/string.h"
#include "type.h"
#include "vector.h"
#include "string.h"
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
