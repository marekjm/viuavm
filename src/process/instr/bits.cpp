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

#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
#include <viua/types/bits.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opbits(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Integer* n = nullptr;
    tie(addr, n) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    *target = unique_ptr<viua::types::Value>{new viua::types::Bits(n->as_unsigned())};

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitat(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Bits* bits = nullptr;
    tie(addr, bits) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Integer* n = nullptr;
    tie(addr, n) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    *target = unique_ptr<viua::types::Value>{new viua::types::Boolean(bits->at(n->as_unsigned()))};

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitset(viua::internals::types::byte* addr) {
    viua::types::Bits* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Integer* index = nullptr;
    tie(addr, index) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    viua::types::Boolean* x = nullptr;
    tie(addr, x) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Boolean>(addr, this);

    target->set(index->as_unsigned(), x->boolean());

    return addr;
}
