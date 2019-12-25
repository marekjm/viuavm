/*
 *  Copyright (C) 2015-2018, 2020 Marek Marecki
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

#include <memory>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>
#include <viua/types/pointer.h>
#include <viua/types/reference.h>


auto viua::process::Process::opmove(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_register(addr, *this);

    *target = std::move(*source);

    return addr;
}
auto viua::process::Process::opcopy(Op_address_type addr) -> Op_address_type
{
    auto target       = decoder.fetch_register(addr, *this);
    auto const source = decoder.fetch_value(addr, *this);

    *target = source->copy();

    return addr;
}
auto viua::process::Process::opptr(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_value(addr, *this);

    *target = source->pointer(this);

    return addr;
}
auto viua::process::Process::opptrlive(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_value_of<viua::types::Pointer>(addr, *this);

    *target = std::make_unique<viua::types::Boolean>(not source->expired());

    return addr;
}
auto viua::process::Process::opswap(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_register(addr, *this);

    target->swap(*source);

    return addr;
}
auto viua::process::Process::opdelete(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);

    if (target->empty()) {
        throw std::make_unique<viua::types::Exception>(
            "delete of null register");
    }
    target->give();

    return addr;
}
auto viua::process::Process::opisnull(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto source = decoder.fetch_register(addr, *this);

    *target = std::make_unique<viua::types::Boolean>(source->empty());

    return addr;
}
