#ifndef TATANKA_TYPES_STRING_H
#define TATANKA_TYPES_STRING_H

#pragma once

#include <string>
#include "object.h"


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
        bool boolean() const {
            return svalue.size() != 0;
        }

        Object* copy() const {
            return new String(svalue);
        }

        std::string& value() { return svalue; }

        String(std::string s = ""): svalue(s) {}

};


#endif
