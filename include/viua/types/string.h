#ifndef VIUA_TYPES_STRING_H
#define VIUA_TYPES_STRING_H

#pragma once

#include <string>
#include "type.h"
#include "vector.h"
#include "integer.h"
#include "../support/string.h"
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>


class String : public Type {
    /** String type.
     *
     *  Designed to hold text.
     */
    std::string svalue;

    public:
        std::string type() const {
            return "String";
        }
        std::string str() const {
            return svalue;
        }
        std::string repr() const {
            return str::enquote(svalue);
        }
        bool boolean() const {
            return svalue.size() != 0;
        }

        Type* copy() const {
            return new String(svalue);
        }

        std::string& value() { return svalue; }

        Integer* size();
        String* sub(int b = 0, int e = -1);
        String* add(String*);
        String* join(Vector*);

        virtual void stringify(Frame*, RegisterSet*, RegisterSet*);
        virtual void represent(Frame*, RegisterSet*, RegisterSet*);

        virtual void startswith(Frame*, RegisterSet*, RegisterSet*);

        virtual void format(Frame*, RegisterSet*, RegisterSet*);
        virtual void substr(Frame*, RegisterSet*, RegisterSet*);
        virtual void concatenate(Frame*, RegisterSet*, RegisterSet*);
        virtual void join(Frame*, RegisterSet*, RegisterSet*);

        virtual void size(Frame*, RegisterSet*, RegisterSet*);

        String(std::string s = ""): svalue(s) {}
};


#endif
