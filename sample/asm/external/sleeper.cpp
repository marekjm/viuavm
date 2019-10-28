/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/value.h>


static auto sleeper_lazy_print(Frame*, viua::kernel::Register_set*, viua::kernel::Register_set*,
                               viua::process::Process*, viua::kernel::Kernel*) -> void {
    std::cout << "sleeper::lazy_print/0: sleep for 5ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    std::cout << "sleeper::lazy_print/0: sleep for 15ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    std::cout << "sleeper::lazy_print/0: sleep for 25ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    std::cout << "sleeper::lazy_print/0: sleep for 10ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::cout << "sleeper::lazy_print/0: sleep for 100ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "sleeper::lazy_print/0: done" << std::endl;
}


const Foreign_function_spec functions[] = {
    {"sleeper::lazy_print/0", &sleeper_lazy_print}, {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() { return functions; }
