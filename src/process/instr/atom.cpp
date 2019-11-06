/*
 *  Copyright (C) 2017, 2018 Marek Marecki
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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/process.h>
#include <viua/support/string.h>
#include <viua/types/atom.h>
#include <viua/types/boolean.h>


auto viua::process::Process::opatom(Op_address_type addr) -> Op_address_type
{
    viua::kernel::Register* target = nullptr;
    std::tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    auto s = std::string{};
    std::tie(addr, s) =
        viua::bytecode::decoder::operands::fetch_primitive_string(addr, this);

    *target = std::make_unique<viua::types::Atom>(str::strdecode(s));

    return addr;
}


auto viua::process::Process::opatomeq(Op_address_type addr) -> Op_address_type
{
    viua::kernel::Register* target = nullptr;
    std::tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Atom *first = nullptr, *second = nullptr;
    std::tie(addr, first) = viua::bytecode::decoder::operands::fetch_object_of<
        std::remove_pointer<decltype(first)>::type>(addr, this);
    std::tie(addr, second) = viua::bytecode::decoder::operands::fetch_object_of<
        std::remove_pointer<decltype(second)>::type>(addr, this);

    *target = std::make_unique<viua::types::Boolean>(*first == *second);

    return addr;
}
