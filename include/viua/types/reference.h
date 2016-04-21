#ifndef VIUA_TYPES_REFERENCE_H
#define VIUA_TYPES_REFERENCE_H

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <viua/types/type.h>

class Reference: public Type {
    Type **pointer;
    unsigned *counter;

    /*  This constructor is used internally by the Reference type to
     *  initialise copies of the reference.
     */
    Reference(Type **ptr, unsigned *ctr): pointer(ptr), counter(ctr) {}

    public:
        virtual std::string type() const;
        virtual std::string str() const;
        virtual std::string repr() const;
        virtual bool boolean() const;

        virtual std::vector<std::string> bases() const;
        virtual std::vector<std::string> inheritancechain() const;

        virtual Reference* copy() const;
        virtual Type* pointsTo() const;
        virtual void rebind(Type*);

        Reference(Type *ptr);
        virtual ~Reference();
};


#endif
