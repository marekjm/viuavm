/*
 *  Copyright (C) 2016 Marek Marecki
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

#ifndef VIUA_PID_H
#define VIUA_PID_H

#pragma once

#include <string>


namespace viua { namespace process {
class Process;

class PID {
    const viua::process::Process* associated_process;

  public:
    bool operator==(viua::process::PID const&) const;
    bool operator==(const viua::process::Process*) const;
    bool operator<(viua::process::PID const&) const;
    bool operator>(viua::process::PID const&) const;

    auto get() const -> decltype(associated_process);
    auto str() const -> std::string;

    PID(const viua::process::Process*);
};
}}  // namespace viua::process


#endif
