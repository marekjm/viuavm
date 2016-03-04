#ifndef VIUA_TYPES_PROCESS_H
#define VIUA_TYPES_PROCESS_H

#pragma once

#include <string>
#include "type.h"
#include "vector.h"
#include "integer.h"
#include "../support/string.h"
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>


// we only need a pointer so class declaration will be sufficient
class Process;


class ProcessType : public Type {
    Process* thrd;

    public:
        std::string type() const;
        std::string str() const;
        std::string repr() const;
        bool boolean() const;
        ProcessType* copy() const;

        virtual void joinable(Frame*, RegisterSet*, RegisterSet*);
        virtual void detach(Frame*, RegisterSet*, RegisterSet*);

        virtual void suspend(Frame*, RegisterSet*, RegisterSet*);
        virtual void wakeup(Frame*, RegisterSet*, RegisterSet*);
        virtual void suspended(Frame*, RegisterSet*, RegisterSet*);

        virtual void getPriority(Frame*, RegisterSet*, RegisterSet*);
        virtual void setPriority(Frame*, RegisterSet*, RegisterSet*);

        virtual void pass(Frame*, RegisterSet*, RegisterSet*);

        bool joinable();
        void join();
        void detach();
        bool stopped();
        bool terminated();
        void transferActiveExceptionTo(Type*&);
        Type* getReturnValue();

        ProcessType(Process* t): thrd(t) {}
};


#endif
