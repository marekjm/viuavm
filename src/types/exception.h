#ifndef VIUA_TYPES_EXCEPTION_H
#define VIUA_TYPES_EXCEPTION_H

#pragma once

#include <string>
#include "object.h"
#include "../support/string.h"


class Exception : public Object {
    /** String type.
     *
     *  Designed to hold text.
     */
    std::string cause;

    public:
        std::string type() const {
            return "Exception";
        }
        std::string str() const {
            return cause;
        }
        std::string repr() const {
            return (type() + ": " + str::enquote(cause));
        }
        bool boolean() const {
            return true;
        }

        Object* copy() const {
            return new Exception(cause);
        }

        virtual std::string what();

        Exception(std::string s = ""): cause(s) {}
};


#endif
