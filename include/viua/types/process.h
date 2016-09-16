/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIUA_TYPES_PROCESS_H
#define VIUA_TYPES_PROCESS_H

#pragma once

#include <string>
#include <memory>
#include <viua/types/type.h>
#include <viua/types/vector.h>
#include <viua/types/integer.h>
#include <viua/support/string.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/process.h>


// we only need a pointer so class declaration will be sufficient
class Kernel;


class ProcessType : public Type {
    Process* thrd;

    public:
        std::string type() const;
        std::string str() const;
        std::string repr() const;
        bool boolean() const;
        ProcessType* copy() const;

        virtual void joinable(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);
        virtual void detach(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);

        virtual void suspend(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);
        virtual void wakeup(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);
        virtual void suspended(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);

        virtual void getPriority(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);
        virtual void setPriority(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*);

        PID pid() const;

        bool joinable();
        void join();
        void detach();
        bool stopped();
        bool terminated();
        std::unique_ptr<Type> transferActiveException();
        std::unique_ptr<Type> getReturnValue();

        ProcessType(Process* t): thrd(t) {}
};


#endif
