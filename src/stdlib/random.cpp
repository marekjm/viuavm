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

#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/float.h>
#include <viua/types/integer.h>
using namespace std;


static auto getrandom() -> long double {
    /** Return random float between 0.0 and 1.0.
     *
     *  This is a utility function exposed to Viua.
     */
    ifstream in("/dev/urandom");
    if (!in) {
        throw make_unique<viua::types::Exception>(
            "failed to open random device: /dev/urandom");
    }
    unsigned long long int rullint = 0;
    in.read(reinterpret_cast<char*>(&rullint), sizeof(rullint));
    return (static_cast<long double>(rullint) /
            static_cast<long double>(ULLONG_MAX));
}

static auto random_drandom(Frame* frame,
                           viua::kernel::RegisterSet*,
                           viua::kernel::RegisterSet*,
                           viua::process::Process*,
                           viua::kernel::Kernel*) -> void {
    /** Return random integer.
     *
     *  Bytes are read from /dev/random random number device.
     *  This call can block if not enough entropy bytes are available.
     */
    ifstream in("/dev/random");
    if (!in) {
        throw make_unique<viua::types::Exception>(
            "failed to open random device: /dev/random");
    }
    int rint = 0;
    in.read(reinterpret_cast<char*>(&rint), sizeof(rint));
    frame->local_register_set->set(0, make_unique<viua::types::Integer>(rint));
}

static auto random_durandom(Frame* frame,
                            viua::kernel::RegisterSet*,
                            viua::kernel::RegisterSet*,
                            viua::process::Process*,
                            viua::kernel::Kernel*) -> void {
    /** Return random integer.
     *
     *  Bytes are read from /dev/urandom random number device.
     *  This is a nonblocking call.
     *  If not enough entropy bytes are available a pseudo-random generator is
     *  employed to supply missing bytes.
     */
    ifstream in("/dev/urandom");
    if (!in) {
        throw make_unique<viua::types::Exception>(
            "failed to open random device: /dev/urandom");
    }
    int rint = 0;
    in.read(reinterpret_cast<char*>(&rint), sizeof(rint));
    frame->local_register_set->set(0, make_unique<viua::types::Integer>(rint));
}

static auto random_random(Frame* frame,
                          viua::kernel::RegisterSet*,
                          viua::kernel::RegisterSet*,
                          viua::process::Process*,
                          viua::kernel::Kernel*) -> void {
    /** Return random float from range between 0.0 and 1.0.
     */
    frame->local_register_set->set(
        0, make_unique<viua::types::Float>(getrandom()));
}

static auto random_randint(Frame* frame,
                           viua::kernel::RegisterSet*,
                           viua::kernel::RegisterSet*,
                           viua::process::Process*,
                           viua::kernel::Kernel*) -> void {
    /** Return random integer from selected range.
     *
     *  Requires two parameters: lower and upper bound.
     *  Returned integer is in range [lower, upper).
     */
    auto lower_bound =
        static_cast<viua::types::Integer*>(frame->arguments->at(0))->value();
    auto upper_bound =
        static_cast<viua::types::Integer*>(frame->arguments->at(1))->value();
    auto modifer = ((upper_bound - lower_bound) * getrandom());
    frame->local_register_set->set(
        0, make_unique<viua::types::Integer>(lower_bound + modifer));
}

const ForeignFunctionSpec functions[] = {
    {"std::random::device::random/0", &random_drandom},
    {"std::random::device::urandom/0", &random_durandom},
    {"std::random::random/0", &random_random},
    {"std::random::randint/2", &random_randint},
    {nullptr, nullptr},
};

extern "C" const ForeignFunctionSpec* exports() { return functions; }
