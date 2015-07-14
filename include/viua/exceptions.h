#ifndef VIUA_EXCEPTIONS_H
#define VIUA_EXCEPTIONS_H

#pragma once

#include <string>
#include "types/exception.h"

class OutOfRangeException: public Exception {
    protected:
        std::string cause;

    public:
        std::string type() const { return "OutOfRangeException"; }
        OutOfRangeException(const std::string& s): cause(s) {}
};

class ReturnStageException: public Exception {
    public:
        std::string type() const { return "ReturnStageException"; }
};


#endif
