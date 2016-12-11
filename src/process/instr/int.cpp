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

#include <memory>
#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/assert.h>
using namespace std;


byte* viua::process::Process::opizero(byte* addr) {
    viua::internals::types::register_index target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    place(target, unique_ptr<viua::types::Type>{new viua::types::Integer(0)});
    return addr;
}

byte* viua::process::Process::opistore(byte* addr) {
    viua::internals::types::register_index target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    int integer = 0;
    tie(addr, integer) = viua::bytecode::decoder::operands::fetch_primitive_int(addr, this);

    place(target, unique_ptr<viua::types::Type>{new viua::types::Integer(integer)});

    return addr;
}

byte* viua::process::Process::opiinc(byte* addr) {
    viua::types::Type* target { nullptr };
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::expect_type<viua::types::Integer>("Integer", target)->increment();

    return addr;
}

byte* viua::process::Process::opidec(byte* addr) {
    viua::types::Type* target { nullptr };
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::expect_type<viua::types::Integer>("Integer", target)->decrement();

    return addr;
}
