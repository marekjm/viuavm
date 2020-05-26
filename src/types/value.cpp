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
#include <sstream>
#include <string>
#include <viua/types/exception.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>


auto viua::types::Value::type() const -> std::string
{ return "Value"; }
std::string viua::types::Value::str() const
{
    std::ostringstream s;
    s << "<'" << type() << "' object at " << this << ">";
    return s.str();
}
std::string viua::types::Value::repr() const { return str(); }
bool viua::types::Value::boolean() const
{
    /*  Boolean defaults to false.
     *  This is because in if, loops etc. we will NOT execute code depending on
     * unknown state. If a derived object overrides this method it is free to
     * return true as it sees fit, but the default is to NOT carry any actions.
     */
    return false;
}


std::unique_ptr<viua::types::Pointer> viua::types::Value::pointer(
    const viua::process::Process* process_of_origin)
{
    return std::make_unique<viua::types::Pointer>(this, process_of_origin);
}

viua::types::Value::~Value()
{
    for (auto p : pointers) {
        p->invalidate(this);
    }
}
