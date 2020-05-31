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

#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>


auto viua::process::Process::opnot(Op_address_type addr) -> Op_address_type
{
    auto target       = decoder.fetch_register(addr, *this);
    auto const source = decoder.fetch_value(addr, *this);

    *target = std::make_unique<viua::types::Boolean>(not source->boolean());

    return addr;
}

using Addr_type = viua::internals::types::Op_address_type;
template<typename Oper>
static auto binary_logical_op(Addr_type addr, viua::process::Process* proc)
    -> Addr_type
{
    auto target    = proc->decoder.fetch_register(addr, *proc);
    auto const lhs = proc->decoder.fetch_value(addr, *proc);
    auto const rhs = proc->decoder.fetch_value(addr, *proc);

    *target = std::make_unique<viua::types::Boolean>(
        Oper{}(lhs->boolean(), rhs->boolean()));

    return addr;
}

auto viua::process::Process::opand(Op_address_type addr) -> Op_address_type
{
    return binary_logical_op<std::logical_and<bool>>(addr, this);
}

auto viua::process::Process::opor(Op_address_type addr) -> Op_address_type
{
    return binary_logical_op<std::logical_or<bool>>(addr, this);
}
