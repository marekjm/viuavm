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

#include <cstdint>
#include <iostream>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/boolean.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* viua::process::Process::opecho(byte* addr) {
    unsigned source = 0;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    cout << fetch(source)->str();
    return addr;
}

byte* viua::process::Process::opprint(byte* addr) {
    unsigned source = 0;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    cout << fetch(source)->str() + '\n';
    return addr;
}


byte* viua::process::Process::opjump(byte* addr) {
    byte* target = (jump_base + viua::bytecode::decoder::operands::extract_primitive_uint64(addr, this));
    if (target == addr) {
        throw new viua::types::Exception("aborting: JUMP instruction pointing to itself");
    }
    return target;
}

byte* viua::process::Process::opif(byte* addr) {
    unsigned source_register_index = 0;
    uint64_t addr_true = 0, addr_false = 0;

    tie(addr, source_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, addr_true) = viua::bytecode::decoder::operands::fetch_primitive_uint64(addr, this);
    tie(addr, addr_false) = viua::bytecode::decoder::operands::fetch_primitive_uint64(addr, this);

    return (jump_base + (fetch(source_register_index)->boolean() ? addr_true : addr_false));
}
