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
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
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
    auto const address_of_this_jump = (addr - 1);

    auto const target = stack->jump_base + decoder.fetch_address(addr);

    if (target == address_of_this_jump) {
        auto const bad_byte = static_cast<uint64_t>(target - stack->jump_base);

        auto o = std::ostringstream{};
        o << "aborting: JUMP instruction pointing to itself at byte ";
        o << bad_byte << " (" << std::hex << "0x" << std::setw(8)
            << std::setfill('0') << bad_byte << ")";
        throw std::make_unique<viua::types::Exception>(o.str());
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
