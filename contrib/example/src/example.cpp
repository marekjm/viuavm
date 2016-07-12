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

#include <time.h>
#include <iostream>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


namespace example {
    void hello_world(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*) {
        cout << "Hello World!" << endl;
    }

    void hello(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
        if (frame->args->size() == 0) {
            throw new Exception("example::hello(String who): expected a parameter, got none");
        }
        cout << "Hello " << frame->args->at(0)->str() << "!" << endl;
    }

    void what_time_is_it(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
        frame->regset->set(0, new Integer(time(nullptr)));
    }
}


// A list describing exported functions.
const ForeignFunctionSpec functions[] = {
    { "example::hello_world/0", example::hello_world },
    { "example::hello/1", example::hello },
    { "example::what_time_is_it/0", example::what_time_is_it },
    { nullptr, nullptr }
};

// A function the VM will call to extract the list of functions and
// their addresses.
extern "C" const ForeignFunctionSpec* exports() { return functions; }
