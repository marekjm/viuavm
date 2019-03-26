/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#ifndef VIUA_TYPES_PROCESS_H
#define VIUA_TYPES_PROCESS_H

#pragma once

#include <memory>
#include <string>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/process.h>
#include <viua/support/string.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>


// we only need a pointer so class declaration will be sufficient
namespace viua { namespace kernel {
class Kernel;
}}  // namespace viua::kernel


namespace viua { namespace types {
class Process : public Value {
    viua::process::Process* thrd;
    viua::process::PID saved_pid;

  public:
    static std::string const type_name;

    /*
     * For use by the VM and user code.
     * Provides interface common to all values in Viua.
     */
    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;
    std::unique_ptr<Value> copy() const override;

    /*
     * For use by the VM.
     * User code has no way of discovering PIDs - it must receive them.
     */
    viua::process::PID pid() const;

    Process(viua::process::Process*);

    auto operator==(Process const&) const -> bool;
};
}}  // namespace viua::types


#endif
