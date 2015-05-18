#ifndef VIUA_TYPES_EXCEPTION_H
#define VIUA_TYPES_EXCEPTION_H

#pragma once

#include <string>
#include "type.h"
#include "../support/string.h"


class Exception : public Type {
    /** String type.
     *
     *  Designed to hold text.
     */
    protected:
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

        Type* copy() const {
            return new Exception(cause);
        }

        virtual std::string what() const;

        Exception(std::string s = ""): cause(s) {}
};


#endif
