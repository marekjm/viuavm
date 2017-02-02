/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/boolean.h>
#include <viua/kernel/kernel.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opnot(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type *source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Boolean(not source->boolean())};

    return addr;
}

viua::internals::types::byte* viua::process::Process::opand(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type *first = nullptr, *second = nullptr;
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Boolean(first->boolean() and second->boolean())};

    return addr;
}

viua::internals::types::byte* viua::process::Process::opor(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type *first = nullptr, *second = nullptr;
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Boolean(first->boolean() or second->boolean())};

    return addr;
}
