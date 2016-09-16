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
#include <viua/types/boolean.h>
#include <viua/kernel/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* Process::opecho(byte* addr) {
    cout << viua::operand::extract(addr)->resolve(this)->str();
    return addr;
}

byte* Process::opprint(byte* addr) {
    addr = opecho(addr);
    cout << '\n';
    return addr;
}


byte* Process::opjump(byte* addr) {
    uint64_t* offset = reinterpret_cast<uint64_t*>(addr);
    byte* target = (jump_base+(*offset));
    if (target == addr) {
        throw new Exception("aborting: JUMP instruction pointing to itself");
    }
    return target;
}

byte* Process::opbranch(byte* addr) {
    Type* condition = viua::operand::extract(addr)->resolve(this);

    uint64_t addr_true, addr_false;
    viua::kernel::util::extractOperand<decltype(addr_true)>(addr, addr_true);
    viua::kernel::util::extractOperand<decltype(addr_false)>(addr, addr_false);

    return (jump_base + (condition->boolean() ? addr_true : addr_false));
}
