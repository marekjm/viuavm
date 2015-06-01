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
        std::string detailed_type;
    public:
        std::string type() const {
            return "Exception";
        }
        std::string str() const {
            return cause;
        }
        std::string repr() const {
            return (etype() + ": " + str::enquote(cause));
        }
        bool boolean() const {
            return true;
        }

        Type* copy() const {
            return new Exception(cause);
        }

        virtual std::string what() const;
        virtual std::string etype() const;

        Exception(std::string s = ""): cause(s), detailed_type("Exception") {}
        Exception(std::string ts, std::string cs): cause(cs), detailed_type(ts) {}
};


#endif
