/*
 *  Copyright (C) 2015, 2016, 2018, 2020 Marek Marecki
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

#include <functional>
#include <viua/assert.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>
#include <viua/types/float.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>
#include <viua/util/memory.h>


auto viua::process::Process::opfloat(Op_address_type addr) -> Op_address_type
{
    auto target      = decoder.fetch_register(addr, *this);
    auto const value = decoder.fetch_float(addr);

    *target = std::make_unique<viua::types::Float>(value);

    return addr;
}
