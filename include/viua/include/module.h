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

#ifndef VIUA_EXMODULE_H
#define VIUA_EXMODULE_H

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/object.h>


const std::vector<std::string> VIUAPATH = {
    ".",
    "~/.local/lib/viua",
    "~/.local/lib/viua/site",
    "/usr/local/lib/viua",
    "/usr/local/lib/viua/site",
    "/usr/lib/viua",
    "/usr/lib/viua/site",
    "" /* this path may cause confusing exception about "failing to extract the interface" from a library if
        * name of Viua lib is the same as one of the system libs, and Viua version has not been found
        */
};


namespace viua {
    namespace process {
        class Process;
    }
    namespace kernel {
        class Kernel;
    }
}


// External functions must have this signature
typedef void (ForeignFunction)(
    Frame*,         // call frame; contains parameters, local registers, name of the function etc.
    viua::kernel::RegisterSet*,   // static register set (may be nullptr)
    viua::kernel::RegisterSet*,   // global register set (may be nullptr)
    viua::process::Process*,       // calling process
    viua::kernel::Kernel*            // VM viua::kernel::Kernel the calling process is running on
);

/** Custom types for Viua VM can be written in C++ and loaded into the typesystem with minimal amount of bookkeeping.
 *  The only thing Viua needs to use a pure-C++ class is a string-name-to-member-function-pointer mapping as
 *  the machine must be able to somehow dispatch the methods.
 *  One downside this approach has is that all method calls are performed via the vtable which may not be the most
 *  efficient way.
 *  Of course, you can also use the struct-and-a-bunch-of-free-functions strategy, in which case you are more interested
 *  in the ForeignFunction typedef defined above.
 */
typedef void (viua::types::Type::*ForeignMethodMemberPointer)(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
typedef std::function<void(viua::types::Type*, Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*)> ForeignMethod;


/** External modules must export the "exports()" function.
 *  Should a module fail to provide this function, it is deemed invalid and is rejected by the VM.
 *
 *  The "exports()" function returns an array of below structures.
 */
struct ForeignFunctionSpec {
    const char* name;
    ForeignFunction* fpointer;
};


#endif
