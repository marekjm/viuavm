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
#include <set>
#include <sstream>
#include <string>

#include <viua/pid.h>
#include <viua/process.h>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>


auto viua::types::Value::type() const -> std::string
{
    return "Value";
}
auto viua::types::Value::str() const -> std::string
{
    std::ostringstream s;
    s << "<'" << type() << "' object at " << this << ">";
    return s.str();
}
auto viua::types::Value::repr() const -> std::string
{
    return str();
}
auto viua::types::Value::boolean() const -> bool
{
    /*  Boolean defaults to false.
     *  This is because in if, loops etc. we will NOT execute code depending on
     * unknown state. If a derived object overrides this method it is free to
     * return true as it sees fit, but the default is to NOT carry any actions.
     */
    return false;
}


auto viua::types::Value::pointer(viua::process::Process* const proc)
    -> std::unique_ptr<viua::types::Pointer>
{
    pointered = proc;
    auto ptr  = std::make_unique<viua::types::Pointer>(this, proc->pid());
    pointered->attach_pointer(this, ptr.get());
    return ptr;
}

auto viua::types::Value::expire() -> void
{
    /*
     * By default do nothing, as values of most types may be safely exchanged
     * between processes.
     */
    if (pointered) {
        pointered->invalidate_pointers_of(this);
    }
}

viua::types::Value::~Value()
{
    expire();
}
