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
#include <climits>
#include <iostream>
#include <fstream>
#include <string>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/exception.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


float getrandom() {
    /** Return random float between 0.0 and 1.0.
     *
     *  This is a utility function exposed to Viua.
     */
    ifstream in("/dev/urandom");
    if (!in) {
        throw new viua::types::Exception("failed to open random device: /dev/urandom");
    }
    unsigned long long int rullint = 0;
    in.read((char*)&rullint, sizeof(unsigned long long int));
    float rfloat = ((long double)rullint / (long double)ULLONG_MAX);
    return rfloat;
}

void random_drandom(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    /** Return random integer.
     *
     *  Bytes are read from /dev/random random number device.
     *  This call can block if not enough entropy bytes are available.
     */
    ifstream in("/dev/random");
    if (!in) {
        throw new viua::types::Exception("failed to open random device: /dev/random");
    }
    int rint = 0;
    in.read((char*)&rint, sizeof(int));
    frame->regset->set(0, unique_ptr<viua::types::Type>{new viua::types::Integer(rint)});
}

void random_durandom(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    /** Return random integer.
     *
     *  Bytes are read from /dev/urandom random number device.
     *  This is a nonblocking call.
     *  If not enough entropy bytes are available a pseudo-random generator is
     *  employed to supply missing bytes.
     */
    ifstream in("/dev/urandom");
    if (!in) {
        throw new viua::types::Exception("failed to open random device: /dev/urandom");
    }
    int rint = 0;
    in.read((char*)&rint, sizeof(int));
    frame->regset->set(0, unique_ptr<viua::types::Type>{new viua::types::Integer(rint)});
}

void random_random(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    /** Return random float from range between 0.0 and 1.0.
     */
    frame->regset->set(0, unique_ptr<viua::types::Type>{new viua::types::Float(getrandom())});
}

void random_randint(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    /** Return random integer from selected range.
     *
     *  Requires two parameters: lower and upper bound.
     *  Returned integer is in range [lower, upper).
     */
    int lower_bound = static_cast<viua::types::Integer*>(frame->args->at(0))->value();
    int upper_bound = static_cast<viua::types::Integer*>(frame->args->at(1))->value();
    int modifer = ((upper_bound - lower_bound) * getrandom());
    frame->regset->set(0, unique_ptr<viua::types::Type>{new viua::types::Integer(lower_bound + modifer)});
}

const ForeignFunctionSpec functions[] = {
    { "std::random::device::random", &random_drandom },
    { "std::random::device::urandom", &random_durandom },
    { "std::random::random", &random_random },
    { "std::random::randint", &random_randint },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
