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

#include <iostream>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/value.h>


static auto hello(Frame*, viua::kernel::Register_set*, viua::kernel::Register_set*, viua::process::Process*,
                  viua::kernel::Kernel*) -> void {
    std::cout << "Hello World!" << std::endl;
}


const Foreign_function_spec functions[] = {
    {"World::print_hello/0", &hello},
    {"World::print_hello/1", &hello}, {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() { return functions; }
