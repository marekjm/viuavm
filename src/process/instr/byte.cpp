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
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/kernel/opex.h>
#include <viua/operand.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* Process::opbstore(byte* addr) {
    unsigned destination_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    bool operand_ref = false;
    char operand;

    operand_ref = *(reinterpret_cast<bool*>(addr));
    pointer::inc<bool, byte>(addr);
    operand = static_cast<char>(*addr);
    ++addr;

    if (operand_ref) {
        operand = static_cast<Byte*>(fetch(static_cast<unsigned>(operand)))->value();
    }

    place(destination_register, new Byte(operand));

    return addr;
}
