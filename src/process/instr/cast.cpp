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

#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/string.h>
#include <viua/types/exception.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* Process::opitof(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(target, new viua::types::Float(static_cast<viua::types::Integer*>(fetch(source))->as_float64()));

    return addr;
}

byte* Process::opftoi(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(target, new viua::types::Integer(static_cast<viua::types::Float*>(fetch(source))->as_int32()));

    return addr;
}

byte* Process::opstoi(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    int result_integer = 0;
    string supplied_string = static_cast<viua::types::String*>(fetch(source))->value();
    try {
        result_integer = std::stoi(supplied_string);
    } catch (const std::out_of_range& e) {
        throw new Exception("out of range: " + supplied_string);
    } catch (const std::invalid_argument& e) {
        throw new Exception("invalid argument: " + supplied_string);
    }

    place(target, new viua::types::Integer(result_integer));

    return addr;
}

byte* Process::opstof(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    string supplied_string = static_cast<viua::types::String*>(fetch(source))->value();
    double convert_from = std::stod(supplied_string);
    place(target, new viua::types::Float(convert_from));

    return addr;
}
