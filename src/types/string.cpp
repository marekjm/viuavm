#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <algorithm>
#include <stdexcept>
#include <viua/support/string.h>
#include <viua/types/type.h>
#include <viua/types/vector.h>
#include <viua/types/object.h>
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
void String::stringify(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() < 2) {
        throw new Exception("expected 2 parameters");
    }
    svalue = frame->args->at(1)->str();
}

void String::represent(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() < 2) {
        throw new Exception("expected 2 parameters");
    }
    svalue = frame->args->at(1)->repr();
}

void String::format(Frame* frame, RegisterSet*, RegisterSet*) {
    regex key_regex("#\\{(?:(?:0|[1-9][0-9]*)|[a-zA-Z_][a-zA-Z0-9_]*)\\}");

    string result = svalue;

    if (regex_search(result, key_regex)) {
        vector<string> matches;
        for (sregex_iterator match = sregex_iterator(result.begin(), result.end(), key_regex); match != sregex_iterator(); ++match) {
            matches.push_back(match->str());
        }

        for (auto i : matches) {
            string m = i.substr(2, (i.size()-3));
            string replacement;
            bool is_number = true;
            int index = -1;
            try {
                index = stoi(m);
            } catch (const std::invalid_argument&) {
                is_number = false;
            }
            if (is_number) {
                replacement = static_cast<Vector*>(frame->args->at(1))->at(index)->str();
            } else {
                replacement = static_cast<Object*>(frame->args->at(2))->at(m)->str();
            }
            string pat("#\\{" + m + "\\}");
            regex subst(pat);
            result = regex_replace(result, subst, replacement);
        }

    }

    frame->regset->set(0, new String(result));
}

void String::sub(Frame* frame, RegisterSet*, RegisterSet*) {
}

void String::concatenate(Frame* frame, RegisterSet*, RegisterSet*) {
    frame->regset->set(0, new String(static_cast<String*>(frame->args->at(0))->value() + static_cast<String*>(frame->args->at(1))->value()));
}

void String::join(Frame* frame, RegisterSet*, RegisterSet*) {
}

void String::size(Frame* frame, RegisterSet*, RegisterSet*) {
    frame->regset->set(0, new Integer(svalue.size()));
}
