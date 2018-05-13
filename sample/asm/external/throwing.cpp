/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <cmath>
#include <iostream>
#include <memory>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/float.h>
#include <viua/types/value.h>
using namespace std;


static auto throwing_oh_noes[[noreturn]](Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*,
                             viua::process::Process*, viua::kernel::Kernel*) -> void {
    throw make_unique<viua::types::Exception>("OH NOES!");
}


const ForeignFunctionSpec functions[] = {
    {"throwing::oh_noes/0", &throwing_oh_noes}, {nullptr, nullptr},
};

extern "C" const ForeignFunctionSpec* exports() { return functions; }
