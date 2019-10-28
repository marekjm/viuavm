/*
 *  Copyright (C) 2019 Marek Marecki
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

#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>  // for close(3), write(3), read(3)
#include <viua/include/module.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/io.h>
#include <viua/types/string.h>


namespace viua { namespace stdlib { namespace posix { namespace io {
template<typename T> auto memset(T& value, int const c) -> void {
    ::memset(&value, c, sizeof(std::remove_reference_t<T>));
}

static auto open(Frame* frame,
                 viua::kernel::Register_set*,
                 viua::kernel::Register_set*,
                 viua::process::Process*,
                 viua::kernel::Kernel*) -> void {
    auto const fd = ::open(frame->arguments->get(0)->str().c_str(), 0);
    if (fd == -1) {
        throw std::make_unique<viua::types::Exception>("Unknown_errno");
    }

    frame->set_local_register_set(
        std::make_unique<viua::kernel::Register_set>(1));
    frame->local_register_set->set(0, std::make_unique<viua::types::IO_fd>(fd));
}
}}}}  // namespace viua::stdlib::posix::io

const Foreign_function_spec functions[] = {
    {"std::posix::io::open/1", &viua::stdlib::posix::io::open},
    {nullptr, nullptr},
};

extern "C" const Foreign_function_spec* exports() {
    return functions;
}
