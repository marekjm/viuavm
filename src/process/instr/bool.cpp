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

#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/boolean.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* Process::opnot(byte* addr) {
    unsigned register_index = 0;
    tie(addr, register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(register_index, new Boolean(not fetch(register_index)->boolean()));

    return addr;
}

byte* Process::opand(byte* addr) {
    unsigned target = 0, first = 0, second = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(target, new Boolean(fetch(first)->boolean() and fetch(second)->boolean()));

    return addr;
}

byte* Process::opor(byte* addr) {
    unsigned target = 0, first = 0, second = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, first) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, second) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(target, new Boolean(fetch(first)->boolean() or fetch(second)->boolean()));

    return addr;
}
