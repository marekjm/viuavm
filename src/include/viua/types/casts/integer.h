#ifndef VIUA_TYPE_INTERFACES_INTEGER_H
#define VIUA_TYPE_INTERFACES_INTEGER_H

#pragma once

#include "../type.h"

class IntegerCast: public Type {
    public:
        virtual int as_integer() const = 0;
        virtual int increment() = 0;
        virtual int decrement() = 0;
};

#endif
