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

#include <iostream>
#include <sstream>
#include <string>

#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/process.h>
#include <viua/types/boolean.h>
#include <viua/types/process.h>


std::string viua::types::Process::type() const
{
    return type_name;
}

std::string viua::types::Process::str() const
{
    return pid().str();
}

std::string viua::types::Process::repr() const
{
    return str();
}

bool viua::types::Process::boolean() const
{
    // There is no good reason why evaluating process as boolean should return
    // either 'false' or 'true', as there is no meaning to this value.
    return false;
}

std::unique_ptr<viua::types::Value> viua::types::Process::copy() const
{
    return std::make_unique<Process>(thrd);
}

viua::process::PID viua::types::Process::pid() const
{
    return saved_pid;
}

viua::types::Process::Process(viua::process::Process* t)
        : thrd(t), saved_pid(thrd->pid())
{}

auto viua::types::Process::operator==(Process const& that) const -> bool
{
    return (saved_pid == that.saved_pid);
}
