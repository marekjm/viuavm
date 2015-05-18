#ifndef VIUA_EXMODULE_H
#define VIUA_EXMODULE_H

#pragma once

#include <vector>
#include "../cpu/frame.h"
#include "../cpu/registerset.h"
#include "../types/type.h"


const std::vector<const char*> VIUAPATH = {
    ".local/lib/viua",
    "/usr/local/lib/viua",
    "/usr/lib/viua",
};


// External functions must have this signature
typedef Type* (ExternalFunction)(Frame*, RegisterSet*, RegisterSet*);

/** External modules must export two functions:
 *
 *      - exports_names: its signature must agree with ExportedFunctionNamesReport typedef,
 *      - exports_pointers: its signature must agree with ExportedFunctionPointersReport typedef,
 *
 *  Should a module fail to provide any of these functions, it is deemed invalid and is rejected by the VM.
 *
 *  The exports_names() function reports names the module exports, and thus defined module's interface.
 *  The exports_pointers() function reports pointers to functions exported by the module.
 *  These two lists MUST BE kept in sync (must be of the same length).
 *  The module is rejected by the VM if they fail to satisfy this requirement.
 *  Both lists are NULL-terminated.
 *
 */
typedef const char** (ExportedFunctionNamesReport)();
typedef ExternalFunction** (ExportedFunctionPointersReport)();


#endif
