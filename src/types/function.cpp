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
#include <string>

#include <viua/types/function.h>
#include <viua/types/value.h>


viua::types::Function::Function(std::string const& name) : function_name(name)
{}

viua::types::Function::~Function()
{}


std::string viua::types::Function::type() const
{
    return type_name;
}

std::string viua::types::Function::str() const
{
    std::ostringstream oss;
    oss << "Function: " << function_name;
    return oss.str();
}

std::string viua::types::Function::repr() const
{
    return str();
}

bool viua::types::Function::boolean() const
{
    return true;
}

std::unique_ptr<viua::types::Value> viua::types::Function::copy() const
{
    return std::make_unique<viua::types::Function>(function_name);
}


std::string viua::types::Function::name() const
{
    return function_name;
}
