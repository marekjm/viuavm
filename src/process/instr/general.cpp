/*
 *  Copyright (C) 2015-2020 Marek Marecki
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

#include <cstdint>
#include <iostream>
#include <memory>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>
#include <viua/util/memory.h>


auto viua::process::Process::opecho(Op_address_type addr) -> Op_address_type
{
    std::cout << decoder.fetch_value(addr, *this)->str();
    return addr;
}

auto viua::process::Process::opprint(Op_address_type addr) -> Op_address_type
{
    std::cout << decoder.fetch_value(addr, *this)->str() + "\n";
    return addr;
}


auto viua::process::Process::opjump(Op_address_type addr) -> Op_address_type
{
    auto const target = stack->jump_base + decoder.fetch_address(addr);

    if (target == addr) {
        throw std::make_unique<viua::types::Exception>(
            "aborting: JUMP instruction pointing to itself");
    }

    return target;
}

auto viua::process::Process::opif(Op_address_type addr) -> Op_address_type
{
    auto const source = decoder.fetch_value(addr, *this)->boolean();

    auto addr_true  = decoder.fetch_address(addr);
    auto addr_false = decoder.fetch_address(addr);

    return (stack->jump_base + (source ? addr_true : addr_false));
}
