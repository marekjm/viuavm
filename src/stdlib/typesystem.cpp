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

#include <memory>
#include <string>
#include <vector>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/string.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
using namespace std;


static auto typeof(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*,
                   viua::process::Process*, viua::kernel::Kernel*) -> void {
    if (not frame->arguments->at(0)) {
        throw make_unique<viua::types::Exception>("expected object as parameter 0");
    }
    frame->local_register_set->set(0, make_unique<viua::types::String>(frame->arguments->get(0)->type()));
}

static auto inheritanceChain(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*,
                             viua::process::Process*, viua::kernel::Kernel*) -> void {
    if (not frame->arguments->at(0)) {
        throw make_unique<viua::types::Exception>("expected object as parameter 0");
    }

    auto ic = frame->arguments->at(0)->inheritancechain();
    auto icv = make_unique<viua::types::Vector>();

    for (decltype(ic)::size_type i = 0; i < ic.size(); ++i) {
        icv->push(make_unique<viua::types::String>(ic[i]));
    }

    frame->local_register_set->set(0, std::move(icv));
}

static auto bases(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*,
                  viua::process::Process*, viua::kernel::Kernel*) -> void {
    if (not frame->arguments->at(0)) {
        throw make_unique<viua::types::Exception>("expected object as parameter 0");
    }

    viua::types::Value* object = frame->arguments->at(0);
    auto ic = object->bases();
    auto icv = make_unique<viua::types::Vector>();

    for (decltype(ic)::size_type i = 0; i < ic.size(); ++i) {
        icv->push(make_unique<viua::types::String>(ic[i]));
    }

    frame->local_register_set->set(0, std::move(icv));
}


const ForeignFunctionSpec functions[] = {
    {"typesystem::typeof/1", &typeof},
    {"typesystem::inheritanceChain/1", &inheritanceChain},
    {"typesystem::bases/1", &bases},
    {nullptr, nullptr},
};

extern "C" const ForeignFunctionSpec* exports() { return functions; }
