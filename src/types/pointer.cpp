/*
 *  Copyright (C) 2015-2017, 2020 Marek Marecki
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

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>


std::string const viua::types::Pointer::type_name = "Pointer";

auto viua::types::Pointer::attach(viua::types::Value* t) -> void
{
    points_to = t;
    points_to->attach_pointer(this);
}
auto viua::types::Pointer::detach() -> void
{
    if (not expired()) {
        points_to->detach_pointer(this);
    }
    points_to = nullptr;
}

void viua::types::Pointer::invalidate(viua::types::Value* t)
{
    if (t == points_to) {
        points_to = nullptr;
    }
}
auto viua::types::Pointer::expired() const -> bool
{
    return (points_to == nullptr);
}
auto viua::types::Pointer::authenticate(viua::process::PID const pid) -> void
{
    /*
     *  Pointers should automatically expire upon crossing process boundaries.
     *  This method should be called before any other method every time the VM
     *  code passes the pointer object to user-process to ensure that Pointer's
     * state is properly accounted for.
     */
    points_to = (origin_pid == pid) ? points_to : nullptr;
}
auto viua::types::Pointer::reset(viua::types::Value* t) -> void
{
    detach();
    attach(t);
}
auto viua::types::Pointer::to(viua::process::PID const p) -> viua::types::Value*
{
    if (origin_pid != p) {
        // Dereferencing pointers outside of their original process is illegal.
        using viua::types::Exception;
        throw std::make_unique<Exception>(Exception::Tag{"Invalid_dereference"},
                                          "outside of original process");
    }
    if (expired()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(Exception::Tag{"Expired_pointer"},
                                          "pointer is no longer valid");
    }
    return points_to;
}

auto viua::types::Pointer::type() const -> std::string
{
    return ((not expired()) ? ("Pointer_of_" + points_to->type())
                            : "Expired_pointer");
}

auto viua::types::Pointer::boolean() const -> bool
{
    return (not expired());
}

auto viua::types::Pointer::str() const -> std::string
{
    return type();
}

auto viua::types::Pointer::copy() const -> std::unique_ptr<viua::types::Value>
{
    if (expired()) {
        return std::make_unique<Pointer>(origin_pid);
    }
    return std::make_unique<Pointer>(points_to, origin_pid);
}


viua::types::Pointer::Pointer(viua::process::PID pid)
        : points_to{nullptr}, origin_pid{pid}
{}
viua::types::Pointer::Pointer(viua::types::Value* t,
                              viua::process::PID const pid)
        : points_to{nullptr}, origin_pid{pid}
{
    attach(t);
}
viua::types::Pointer::~Pointer()
{
    detach();
}
