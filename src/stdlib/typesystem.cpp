/*
 *  Copyright (C) 2015-2018, 2020 Marek Marecki
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
#include <viua/process.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>


static auto typeof(Frame* frame,
                   viua::kernel::Register_set*,
                   viua::kernel::Register_set*,
                   viua::process::Process* process,
                   viua::kernel::Kernel*) -> void
{
    if (not frame->arguments->at(0)) {
        throw std::make_unique<viua::types::Exception>("requires 1 parameter");
    }
    if (auto const pointer =
            dynamic_cast<viua::types::Pointer*>(frame->arguments->get(0));
        pointer) {
        frame->set_local_register_set(
            std::make_unique<viua::kernel::Register_set>(1));
        frame->local_register_set->set(0,
                                       std::make_unique<viua::types::String>(
                                           pointer->to(*process)->type()));
    } else {
        throw std::make_unique<viua::types::Exception>(
            "expected a pointer as parameter 0");
    }
}


const Foreign_function_spec functions[] = {
    {"std::typesystem::typeof/1", &typeof},
    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports()
{
    return functions;
}
