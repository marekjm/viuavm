/*
 *  Copyright (C) 2016 Marek Marecki
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

#include <iostream>
#include <sstream>
#include <memory>
#include <viua/types/type.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


extern "C" const ForeignFunctionSpec* exports();


static void printer_print(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    unique_ptr<viua::types::Type> arg(frame->args->pop(0));
    // concatenate before printing to avoid mangled output
    cout << ("Hello " + arg->str() + "!\n");
}


const ForeignFunctionSpec functions[] = {
    { "printer::print/1", &printer_print },
    { nullptr, nullptr },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
