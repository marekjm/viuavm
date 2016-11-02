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

#include <cmath>
#include <iostream>
#include <viua/types/type.h>
#include <viua/types/float.h>
#include <viua/types/exception.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


extern "C" const ForeignFunctionSpec* exports();


static void math_sqrt(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->args->at(0) == nullptr) {
        throw new viua::types::Exception("expected float as first argument");
    }
    if (frame->args->at(0)->type() != "Float") {
        throw new viua::types::Exception("invalid type of parameter 0: expected Float");
    }

    auto square_root = sqrt(dynamic_cast<viua::types::numeric::Number*>(frame->args->at(0))->as_float64());
    frame->regset->set(0, new viua::types::Float(square_root));
}


const ForeignFunctionSpec functions[] = {
    { "math::sqrt/1", &math_sqrt },
    { nullptr, nullptr },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
