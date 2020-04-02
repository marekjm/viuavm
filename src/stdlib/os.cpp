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
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <viua/include/module.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/struct.h>
#include <viua/types/boolean.h>
#include <viua/types/value.h>


static void os_system(Frame* frame,
                      viua::kernel::Register_set*,
                      viua::kernel::Register_set*,
                      viua::process::Process*,
                      viua::kernel::Kernel*)
{
    if (frame->arguments->at(0) == nullptr) {
        throw std::make_unique<viua::types::Exception>(
            "expected command to launch (string) as parameter 0");
    }
    auto const command = frame->arguments->get(0)->str();
    auto const ret     = system(command.c_str());
    frame->local_register_set->set(0,
                                   std::make_unique<viua::types::Integer>(ret));
}

static void os_lsdir(Frame* frame,
                     viua::kernel::Register_set*,
                     viua::kernel::Register_set*,
                     viua::process::Process*,
                     viua::kernel::Kernel*)
{
    auto const path = frame->arguments->at(0)->str();

    auto entries = std::make_unique<viua::types::Vector>();

    for (auto const& each : std::filesystem::directory_iterator{path}) {
        auto entry = std::make_unique<viua::types::Struct>();
        entry->insert("path", std::make_unique<viua::types::String>(each.path()));
        entry->insert("is_directory",
            viua::types::Boolean::make(std::filesystem::is_directory(each.status())));
        entry->insert("is_regular_file",
            viua::types::Boolean::make(std::filesystem::is_regular_file(each.status())));
        entries->push(std::move(entry));
    }

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::move(entries));
}


const Foreign_function_spec functions[] = {
    {"std::os::system/1", &os_system},
    {"std::os::lsdir/1", &os_lsdir},
    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() { return functions; }
