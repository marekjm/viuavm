#ifndef VIUA_EXMODULE_H
#define VIUA_EXMODULE_H

#pragma once

#include <vector>
#include "../cpu/frame.h"
#include "../cpu/registerset.h"
#include "../types/type.h"


const std::vector<const char*> VIUAPATH = {
    ".",
    "~/.local/lib/viua",
    "/usr/local/lib/viua",
    "/usr/lib/viua",
};



/** External modules must export the "exports()" function.
 *  Should a module fail to provide this function, it is deemed invalid and is rejected by the VM.
 */

// External functions must have this signature
typedef Type* (ForeignFunction)(Frame*, RegisterSet*, RegisterSet*);

// Specification of single external function
// The "exports()" function returns an array of such structures.
struct ForeignFunctionSpec {
    const char* name;
    ForeignFunction* fpointer;
};


#endif
