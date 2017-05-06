/*
 *  Copyright (C) 2017 Marek Marecki
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

#include <viua/support/string.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/process.h>
#include <viua/types/atom.h>
#include <viua/types/boolean.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opatom(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    string s;
    tie(addr, s) = viua::bytecode::decoder::operands::fetch_primitive_string(addr, this);

    *target = unique_ptr<viua::types::Value>{new viua::types::Atom(str::strdecode(s))};

    return addr;
}


viua::internals::types::byte* viua::process::Process::opatomeq(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Atom *first = nullptr, *second = nullptr;
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_object_of<std::remove_pointer<decltype(first)>::type>(addr, this);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_object_of<std::remove_pointer<decltype(second)>::type>(addr, this);

    *target = unique_ptr<viua::types::Value>{new viua::types::Boolean(*first == *second)};

    return addr;
}
