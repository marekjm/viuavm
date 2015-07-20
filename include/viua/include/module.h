#ifndef VIUA_EXMODULE_H
#define VIUA_EXMODULE_H

#pragma once

#include <vector>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/types/type.h>


const std::vector<const char*> VIUAPATH = {
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
};



/** External modules must export the "exports()" function.
 *  Should a module fail to provide this function, it is deemed invalid and is rejected by the VM.
 */

// External functions must have this signature
typedef Type* (ExternalFunction)(Frame*, RegisterSet*, RegisterSet*);

// Specification of single external function
// The "exports()" function returns an array of such structures.
struct ExternalFunctionSpec {
    const char* name;
    ExternalFunction* fpointer;
};


#endif
