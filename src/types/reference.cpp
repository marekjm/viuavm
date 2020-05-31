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

#include <sstream>

#include <viua/types/reference.h>


std::string const viua::types::Reference::type_name = "Reference";

viua::types::Value* viua::types::Reference::points_to() const
{
    return *pointer;
}

void viua::types::Reference::rebind(viua::types::Value* ptr)
{
    if (*pointer) {
        delete (*pointer);
    }
    (*pointer) = ptr;
}

void viua::types::Reference::rebind(std::unique_ptr<viua::types::Value> ptr)
{
    if (*pointer) {
        delete (*pointer);
    }
    (*pointer) = ptr.release();
}


std::string viua::types::Reference::type() const
{
    return (*pointer)->type();
}

std::string viua::types::Reference::str() const
{
    return (*pointer)->str();
}
std::string viua::types::Reference::repr() const
{
    return (*pointer)->repr();
}
bool viua::types::Reference::boolean() const
{
    return (*pointer)->boolean();
}

std::unique_ptr<viua::types::Value> viua::types::Reference::copy() const
{
    ++(*counter);
    return std::make_unique<viua::types::Reference>(pointer, counter);
}

viua::types::Reference::Reference(viua::types::Value* ptr)
        : pointer(new viua::types::Value*(ptr)), counter(new uint64_t{1})
{}
viua::types::Reference::Reference(Value** ptr, uint64_t* ctr)
        : pointer(ptr), counter(ctr)
{}
viua::types::Reference::~Reference()
{
    /** Copies of the reference may be freely spawned and destroyed, but
     *  the internal object *MUST* be preserved until its refcount reaches zero.
     */
    if ((--(*counter)) == 0) {
        delete *pointer;
        delete pointer;
        delete counter;
    }
}
