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

#include <string>
#include <vector>
#include <viua/types/type.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


void typeof(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->args->at(0) == 0) {
        throw new viua::types::Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new viua::types::String(frame->args->get(0)->type()));
}

void inheritanceChain(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->args->at(0) == 0) {
        throw new viua::types::Exception("expected object as parameter 0");
    }

    vector<string> ic = frame->args->at(0)->inheritancechain();
    viua::types::Vector* icv = new viua::types::Vector();

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(new viua::types::String(ic[i]));
    }

    frame->regset->set(0, icv);
}

void bases(Frame* frame, RegisterSet*, RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->args->at(0) == 0) {
        throw new viua::types::Exception("expected object as parameter 0");
    }

    viua::types::Type* object = frame->args->at(0);
    vector<string> ic = object->bases();
    viua::types::Vector* icv = new viua::types::Vector();

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(new viua::types::String(ic[i]));
    }

    frame->regset->set(0, icv);
}


const ForeignFunctionSpec functions[] = {
    { "typesystem::typeof/1", &typeof },
    { "typesystem::inheritanceChain/1", &inheritanceChain },
    { "typesystem::bases/1", &bases },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
