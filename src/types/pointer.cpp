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

#include <viua/process.h>
#include <viua/types/boolean.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>


auto viua::types::Pointer::expire() -> void
{
    points_to = nullptr;
}
auto viua::types::Pointer::expired(viua::process::Process const& proc) -> bool
{
    return (not proc.verify_liveness(*this));
}
auto viua::types::Pointer::authenticate(viua::process::PID const pid) -> void
{
    /*
     *  Pointers should automatically expire upon crossing process boundaries.
     *  This method should be called before any other method every time the VM
     *  code passes the pointer object to user-process to ensure that Pointer's
     * state is properly accounted for.
     */
    if (pid != origin) {
        expire();
    }
}
auto viua::types::Pointer::to(viua::process::Process const& p) -> Value*
{
    if (origin != p.pid()) {
        // Dereferencing pointers outside of their original process is illegal.
        expire();
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"Invalid_dereference"},
            "outside of original process");
    }
    if (not p.verify_liveness(*this)) {
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"Expired_pointer"});
    }
    if (points_to == nullptr) {
        throw std::make_unique<viua::types::Exception>(
            viua::types::Exception::Tag{"Expired_pointer"});
    }
    return points_to;
}
auto viua::types::Pointer::of() const -> Value*
{
    return points_to;
}

auto viua::types::Pointer::type() const -> std::string
{
    return "Pointer";
    // FIXME The below code would require to() to work on const pointers.
    /* auto const& val = to(); */
    /* return (val.has_value() */
    /*     ? ("Pointer_of_" + val->type()) */
    /*     : "Expired_pointer"); */
}

auto viua::types::Pointer::boolean() const -> bool
{
    return (points_to != nullptr);
}

auto viua::types::Pointer::str() const -> std::string
{
    return type();
}

auto viua::types::Pointer::copy() const -> std::unique_ptr<Value>
{
    if (points_to == nullptr /* and (origin == proc) */) {
        // FIXME Make copying a expired pointer an exception?
        return std::make_unique<Pointer>(origin);
    }
    return std::make_unique<Pointer>(points_to, origin);
}


viua::types::Pointer::Pointer(viua::process::PID const pid)
        : points_to{nullptr}, origin{pid}
{}
viua::types::Pointer::Pointer(viua::types::Value* t,
                              viua::process::PID const pid)
        : points_to{t}, origin{pid}
{}
viua::types::Pointer::~Pointer()
{}
