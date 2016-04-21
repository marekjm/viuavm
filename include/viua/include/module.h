#ifndef VIUA_EXMODULE_H
#define VIUA_EXMODULE_H

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
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


// forward declarations since only pointers are needed
class Process;
class CPU;


// External functions must have this signature
typedef void (ExternalFunction)(
    Frame*,         // call frame; contains parameters, local registers, name of the function etc.
    RegisterSet*,   // static register set (may be nullptr)
    RegisterSet*,   // global register set (may be nullptr)
    Process*,       // calling process
    CPU*            // VM CPU the calling process is running on
);

/** Custom types for Viua VM can be written in C++ and loaded into the typesystem with minimal amount of bookkeeping.
 *  The only thing Viua needs to use a pure-C++ class is a string-name-to-member-function-pointer mapping as
 *  the machine must be able to somehow dispatch the methods.
 *  One downside this approach has is that all method calls are performed via the vtable which may not be the most
 *  efficient way.
 *  Of course, you can also use the struct-and-a-bunch-of-free-functions strategy, in which case you are more interested
 *  in the ExternalFunction typedef defined above.
 */
typedef void (Type::*ForeignMethodMemberPointer)(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);
typedef std::function<void(Type*, Frame*, RegisterSet*, RegisterSet*, Process*, CPU*)> ForeignMethod;


/** External modules must export the "exports()" function.
 *  Should a module fail to provide this function, it is deemed invalid and is rejected by the VM.
 *
 *  The "exports()" function returns an array of below structures.
 */
struct ExternalFunctionSpec {
    const char* name;
    ExternalFunction* fpointer;
};


#endif
