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

#include <cstdlib>
#include <memory>
#include <string>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
using namespace std;


static void os_system(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*,
                      viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->arguments->at(0) == nullptr) {
        throw new viua::types::Exception("expected command to launch (string) as parameter 0");
    }
    string command = frame->arguments->get(0)->str();
    int ret = system(command.c_str());
    frame->local_register_set->set(0, make_unique<viua::types::Integer>(ret));
}


const ForeignFunctionSpec functions[] = {
    {"os::system", &os_system},
    {nullptr, nullptr},
};

extern "C" const ForeignFunctionSpec* exports() { return functions; }
