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
#include <thread>
#include <chrono>
#include <viua/types/type.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


extern "C" const ForeignFunctionSpec* exports();


static void sleeper_lazy_print(Frame*, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    cout << "sleeper::lazy_print/0: sleep for 5ms" << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    cout << "sleeper::lazy_print/0: sleep for 15ms" << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(15));

    cout << "sleeper::lazy_print/0: sleep for 25ms" << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    cout << "sleeper::lazy_print/0: sleep for 10ms" << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    cout << "sleeper::lazy_print/0: sleep for 100ms" << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    cout << "sleeper::lazy_print/0: done" << endl;
}


const ForeignFunctionSpec functions[] = {
    { "sleeper::lazy_print/0", &sleeper_lazy_print },
    { nullptr, nullptr },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
