/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <algorithm>
#include <stdexcept>
#include <viua/support/string.h>
#include <viua/types/type.h>
#include <viua/types/pointer.h>
#include <viua/types/boolean.h>
#include <viua/types/vector.h>
#include <viua/types/object.h>
#include <viua/types/string.h>
#include <viua/assert.h>
#include <viua/exceptions.h>
using namespace std;
using namespace viua::assertions;


Integer* String::size() {
    /** Return size of the string.
     */
    return new Integer(int(svalue.size()));
}

String* String::sub(int b, int e) {
    /** Return substring extracted from this object.
     */
    string::size_type cut_from, cut_to;
    // these casts are ugly as hell, but without them Clang warns about implicit sign-changing
    if (b < 0) {
        cut_from = (svalue.size() - static_cast<unsigned>(-b));
    } else {
        cut_from = static_cast<decltype(cut_from)>(b);
    }
    if (e < 0) {
        cut_to = (svalue.size() - static_cast<unsigned>(-e) + 1);
    } else {
        cut_to = static_cast<decltype(cut_to)>(e);
    }
    return new String(svalue.substr(cut_from, cut_to));
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
void String::stringify(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    if (frame->args->size() < 2) {
        throw new Exception("expected 2 parameters");
    }
    svalue = static_cast<Pointer*>(frame->args->at(1))->to()->str();
}

void String::represent(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    if (frame->args->size() < 2) {
        throw new Exception("expected 2 parameters");
    }
    svalue = static_cast<Pointer*>(frame->args->at(1))->to()->repr();
}

void String::startswith(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    string s = static_cast<String*>(frame->args->at(1))->value();
    bool starts_with = false;

    if (s.size() <= svalue.size()) {
        long unsigned i = 0;
        while (i < s.size()) {
            if (!(starts_with = (s[i] == svalue[i]))) {
                break;
            }
            ++i;
        }
    }

    frame->regset->set(0, new Boolean(starts_with));
}

void String::endswith(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    string s = static_cast<String*>(frame->args->at(1))->value();
    bool ends_with = false;

    if (s.size() <= svalue.size()) {
        auto i = s.size();
        auto j = svalue.size();
        while (i > 0) {
            if (!(ends_with = (s[i] == svalue[j]))) {
                break;
            }
            --i;
            --j;
        }
    }

    frame->regset->set(0, new Boolean(ends_with));
}

void String::format(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    regex key_regex("#\\{(?:(?:0|[1-9][0-9]*)|[a-zA-Z_][a-zA-Z0-9_]*)\\}");

    string result = svalue;

    if (regex_search(result, key_regex)) {
        vector<string> matches;
        for (sregex_iterator match = sregex_iterator(result.begin(), result.end(), key_regex); match != sregex_iterator(); ++match) {
            matches.emplace_back(match->str());
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

void String::substr(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    int begin = 0;
    int end = -1;

    assert_arity(frame, 1ul, 2ul, 3ul);

    if (frame->args->size() > 1) {
        assert_typeof(frame->args->at(1), "Integer");
        if (Integer* i = dynamic_cast<Integer*>(frame->args->at(1))) {
            begin = i->value();
        }
    }
    if (frame->args->size() > 2) {
        assert_typeof(frame->args->at(2), "Integer");
        if (Integer* i = dynamic_cast<Integer*>(frame->args->at(2))) {
            end = i->value();
        }
    }
    frame->regset->set(0, sub(begin, end));
}

void String::concatenate(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    frame->regset->set(0, new String(static_cast<String*>(frame->args->at(0))->value() + static_cast<String*>(frame->args->at(1))->value()));
}

void String::join(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    // TODO: implement
}

void String::size(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    frame->regset->set(0, new Integer(static_cast<int>(svalue.size())));
}
