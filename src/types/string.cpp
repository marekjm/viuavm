/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <algorithm>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <viua/assert.h>
#include <viua/exceptions.h>
#include <viua/support/string.h>
#include <viua/types/boolean.h>
#include <viua/types/object.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
using namespace std;
using namespace viua::assertions;
using namespace viua::types;

const std::string viua::types::String::type_name = "String";

std::string String::type() const {
    return "String";
}
std::string String::str() const {
    return svalue;
}
std::string String::repr() const {
    return str::enquote(svalue);
}
bool String::boolean() const {
    return svalue.size() != 0;
}

unique_ptr<Value> String::copy() const {
    return make_unique<String>(svalue);
}

std::string& String::value() {
    return svalue;
}

Integer* String::size() {
    /** Return size of the string.
     */
    return new Integer(static_cast<Integer::underlying_type>(svalue.size()));
}

String* String::sub(int64_t b, int64_t e) {
    /** Return substring extracted from this object.
     */
    std::string::size_type cut_from, cut_to;
    // these casts are ugly as hell, but without them Clang warns about implicit
    // sign-changing
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
    std::string s  = "";
    int vector_len = v->len();
    for (int i = 0; i < vector_len; ++i) {
        s += v->at(i)->str();
        if (i < (vector_len - 1)) {
            s += svalue;
        }
    }
    return new String(s);
}

// foreign methods
void String::stringify(Frame* frame,
                       viua::kernel::RegisterSet*,
                       viua::kernel::RegisterSet*,
                       viua::process::Process* process,
                       viua::kernel::Kernel*) {
    if (frame->arguments->size() < 2) {
        throw make_unique<viua::types::Exception>("expected 2 parameters");
    }
    svalue = static_cast<Pointer*>(frame->arguments->at(1))->to(process)->str();
}

void String::represent(Frame* frame,
                       viua::kernel::RegisterSet*,
                       viua::kernel::RegisterSet*,
                       viua::process::Process* process,
                       viua::kernel::Kernel*) {
    if (frame->arguments->size() < 2) {
        throw make_unique<viua::types::Exception>("expected 2 parameters");
    }
    svalue =
        static_cast<Pointer*>(frame->arguments->at(1))->to(process)->repr();
}

void String::startswith(Frame* frame,
                        viua::kernel::RegisterSet*,
                        viua::kernel::RegisterSet*,
                        viua::process::Process*,
                        viua::kernel::Kernel*) {
    std::string s    = static_cast<String*>(frame->arguments->at(1))->value();
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

    frame->local_register_set->set(
        0, make_unique<viua::types::Boolean>(starts_with));
}

void String::endswith(Frame* frame,
                      viua::kernel::RegisterSet*,
                      viua::kernel::RegisterSet*,
                      viua::process::Process*,
                      viua::kernel::Kernel*) {
    std::string s  = static_cast<String*>(frame->arguments->at(1))->value();
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

    frame->local_register_set->set(
        0, make_unique<viua::types::Boolean>(ends_with));
}

void String::format(Frame* frame,
                    viua::kernel::RegisterSet*,
                    viua::kernel::RegisterSet*,
                    viua::process::Process*,
                    viua::kernel::Kernel*) {
    regex key_regex("#\\{(?:(?:0|[1-9][0-9]*)|[a-zA-Z_][a-zA-Z0-9_]*)\\}");

    std::string result = svalue;

    if (regex_search(result, key_regex)) {
        vector<std::string> matches;
        for (sregex_iterator match =
                 sregex_iterator(result.begin(), result.end(), key_regex);
             match != sregex_iterator();
             ++match) {
            matches.emplace_back(match->str());
        }

        for (auto i : matches) {
            std::string m = i.substr(2, (i.size() - 3));
            std::string replacement;
            bool is_number = true;
            int index      = -1;
            try {
                index = stoi(m);
            } catch (const std::invalid_argument&) {
                is_number = false;
            }
            if (is_number) {
                replacement = static_cast<Vector*>(frame->arguments->at(1))
                                  ->at(index)
                                  ->str();
            } else {
                replacement =
                    static_cast<Object*>(frame->arguments->at(2))->at(m)->str();
            }
            std::string pat("#\\{" + m + "\\}");
            regex subst(pat);
            result = regex_replace(result, subst, replacement);
        }
    }

    frame->local_register_set->set(0, make_unique<String>(result));
}

void String::substr(Frame* frame,
                    viua::kernel::RegisterSet*,
                    viua::kernel::RegisterSet*,
                    viua::process::Process*,
                    viua::kernel::Kernel*) {
    Integer::underlying_type begin = 0;
    Integer::underlying_type end   = -1;

    assert_arity(frame, 1u, 2u, 3u);

    if (frame->arguments->size() > 1) {
        assert_typeof(frame->arguments->at(1), "Integer");
        if (Integer* i = dynamic_cast<Integer*>(frame->arguments->at(1))) {
            begin = i->as_integer();
        }
    }
    if (frame->arguments->size() > 2) {
        assert_typeof(frame->arguments->at(2), "Integer");
        if (Integer* i = dynamic_cast<Integer*>(frame->arguments->at(2))) {
            end = i->as_integer();
        }
    }
    frame->local_register_set->set(
        0, unique_ptr<viua::types::Value>{sub(begin, end)});
}

void String::concatenate(Frame* frame,
                         viua::kernel::RegisterSet*,
                         viua::kernel::RegisterSet*,
                         viua::process::Process*,
                         viua::kernel::Kernel*) {
    frame->local_register_set->set(
        0,
        make_unique<String>(
            static_cast<String*>(frame->arguments->at(0))->value()
            + static_cast<String*>(frame->arguments->at(1))->value()));
}

void String::join(Frame*,
                  viua::kernel::RegisterSet*,
                  viua::kernel::RegisterSet*,
                  viua::process::Process*,
                  viua::kernel::Kernel*) {
    // TODO: implement
}

void String::size(Frame* frame,
                  viua::kernel::RegisterSet*,
                  viua::kernel::RegisterSet*,
                  viua::process::Process*,
                  viua::kernel::Kernel*) {
    frame->local_register_set->set(
        0, make_unique<Integer>(static_cast<int>(svalue.size())));
}

String::String(std::string s) : svalue(s) {}
