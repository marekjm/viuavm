#ifndef TATANKA_TYPES_STRING_H
#define TATANKA_TYPES_STRING_H

#pragma once

#include <string>
#include "object.h"
#include "vector.h"
#include "integer.h"
#include "../support/string.h"


class String : public Object {
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

        Object* copy() const {
            return new String(svalue);
        }

        std::string& value() { return svalue; }

        Integer* size();
        String* sub(int b = 0, int e = -1);
        String* add(String*);
        String* join(Vector*);

        String(std::string s = ""): svalue(s) {}
};


#endif
