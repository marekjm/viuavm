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
    "~/.local/lib/viua/core",
    "~/.local/lib/viua/site",
    "/usr/local/lib/viua",
    "/usr/local/lib/viua/core",
    "/usr/local/lib/viua/site",
    "/usr/lib/viua",
    "/usr/lib/viua/core",
    "/usr/lib/viua/site",
    "" /* this path may cause confusing exception about "failing to extract the interface" from a library if
        * name of Viua lib is the same as one of the system libs, and Viua version has not been found
        */
};



/** External modules must export the "exports()" function.
 *  Should a module fail to provide this function, it is deemed invalid and is rejected by the VM.
 */

// External functions must have this signature
typedef Type* (ExternalFunction)(Frame*, RegisterSet*, RegisterSet*);

/** Custom types for Viua VM can be written in C++ and loaded into the typesystem with minimal amount of bookkeeping.
 *  The only thing Viua needs to use a pure-C++ class is a string-name-to-member-function-pointer mapping as
 *  the machine must be able to somehow dispatch the methods.
 *  One downside this approach has is that all method calls are performed via the vtable which may not be the most
 *  efficient way.
 */
typedef Type* (Type::*ForeignMethodMemberPointer)(Frame*, RegisterSet*, RegisterSet*);
typedef std::function<Type*(Type*, Frame*, RegisterSet*, RegisterSet*)> ForeignMethod;

// Specification of single external function
// The "exports()" function returns an array of such structures.
struct ExternalFunctionSpec {
    const char* name;
    ExternalFunction* fpointer;
};


#endif
