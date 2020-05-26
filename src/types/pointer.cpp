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

void viua::types::Pointer::attach()
{
    points_to->pointers.push_back(this);
}
void viua::types::Pointer::detach()
{
    if (not expired()) {
        points_to->pointers.erase(std::find(
            points_to->pointers.begin(), points_to->pointers.end(), this));
    }
    points_to = nullptr;
}

void viua::types::Pointer::invalidate(viua::types::Value* t)
{
    if (t == points_to) {
        points_to = nullptr;
    }
}
bool viua::types::Pointer::expired() const
{
    return (points_to == nullptr);
}
auto viua::types::Pointer::authenticate(const viua::process::Process* process)
    -> void
{
    /*
     *  Pointers should automatically expire upon crossing process boundaries.
     *  This method should be called before any other method every time the VM
     *  code passes the pointer object to user-process to ensure that Pointer's
     * state is properly accounted for.
     */
    points_to = (process_of_origin == process)
        ? points_to
        : nullptr;
}
void viua::types::Pointer::reset(viua::types::Value* t)
{
    detach();
    points_to = t;
    attach();
}
viua::types::Value* viua::types::Pointer::to(const viua::process::Process* p)
{
    if (process_of_origin != p) {
        // Dereferencing pointers outside of their original process is illegal.
        throw std::make_unique<viua::types::Exception>(
            "InvalidDereference: outside of original process");
    }
    if (expired()) {
        throw std::make_unique<viua::types::Exception>(
            "expired pointer exception");
    }
    return points_to;
}

std::string viua::types::Pointer::type() const
{
    return (((not expired()) ? points_to->type() : "Expired") + "Pointer");
}

auto viua::types::Pointer::boolean() const -> bool
{ return (not expired()); }

std::string viua::types::Pointer::str() const { return type(); }

std::unique_ptr<viua::types::Value> viua::types::Pointer::copy() const
{
    if (expired()) {
        return std::make_unique<Pointer>(process_of_origin);
    }
    return std::make_unique<Pointer>(points_to, process_of_origin);
}


viua::types::Pointer::Pointer(const viua::process::Process* poi)
    : points_to{nullptr}
    , process_of_origin{poi}
{}
viua::types::Pointer::Pointer(viua::types::Value* t,
                              const viua::process::Process* poi)
    : points_to{t}
    , process_of_origin{poi}
{
    attach();
}
viua::types::Pointer::~Pointer() { detach(); }
