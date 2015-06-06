#ifndef VIUA_TYPE_FUNCTION_H
#define VIUA_TYPE_FUNCTION_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "../cpu/registerset.h"
#include "type.h"


class Function : public Type {
    /** Type representing a function.
     */
    public:
        std::string function_name;

        virtual std::string type() const;
        virtual std::string str() const;
        virtual std::string repr() const;

        virtual bool boolean() const;

        virtual Type* copy() const;

        // FIXME: implement real dtor
        Function();
        virtual ~Function();
};


#endif
