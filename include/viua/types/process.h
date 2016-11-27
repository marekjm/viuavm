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
namespace viua {
    namespace kernel {
        class Kernel;
    }
}


namespace viua {
    namespace types {
        class Process : public Type {
            viua::process::Process* thrd;

            public:
                /*
                 * For use by the VM and user code.
                 * Provides interface common to all values in Viua.
                 */
                std::string type() const;
                std::string str() const;
                std::string repr() const;
                bool boolean() const;
                std::unique_ptr<Type> copy() const override;

                /*
                 * For use by user code.
                 * Users should be able to check if a process is joinable, and
                 * to detach a process.
                 */
                virtual void joinable(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
                virtual void detach(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);

                /*
                 * For use by the VM.
                 * User code has no way of discovering PIDs - it must receive them.
                 */
                viua::process::PID pid() const;

                /*
                 * For use by the VM.
                 * Use code *must not* touch these functions.
                 * Well, technically, it can - e.g. via a FFI calls to libraries hooking into the VM but
                 * then all bets are off.
                 */
                void join();
                bool joinable();
                void detach();
                bool stopped();
                bool terminated();
                std::unique_ptr<Type> transferActiveException();
                std::unique_ptr<Type> getReturnValue();

                Process(viua::process::Process* t): thrd(t) {}
        };
    }
}


#endif
