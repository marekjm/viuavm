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
