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

#include <viua/bytecode/decoder/operands.h>
#include <viua/types/pointer.h>
#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/kernel/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* Process::opmove(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    uregset->move(source, target);

    return addr;
}
byte* Process::opcopy(byte* addr) {
    unsigned target = 0;
    Type *source = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    place(target, source->copy());

    return addr;
}
byte* Process::opptr(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, viua::operand::extract(addr)->resolve(this)->pointer());

    return addr;
}
byte* Process::opswap(byte* addr) {
    unsigned first = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned second = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    uregset->swap(first, second);

    return addr;
}
byte* Process::opdelete(byte* addr) {
    uregset->free(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this));
    return addr;
}
byte* Process::opisnull(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, new Boolean(uregset->at(source) == nullptr));

    return addr;
}

byte* Process::opress(byte* addr) {
    /*  Run ress instruction.
     */
    int to_register_set = 0;
    viua::kernel::util::extractOperand<decltype(to_register_set)>(addr, to_register_set);

    switch (to_register_set) {
        case 0:
            uregset = regset.get();
            break;
        case 1:
            uregset = frames.back()->regset;
            break;
        case 2:
            ensureStaticRegisters(frames.back()->function_name);
            uregset = static_registers.at(frames.back()->function_name).get();
            break;
        case 3:
            // TODO: switching to temporary registers
        default:
            throw new Exception("illegal register set ID in ress instruction");
    }

    return addr;
}

byte* Process::optmpri(byte* addr) {
    tmp.reset(pop(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this)));
    return addr;
}
byte* Process::optmpro(byte* addr) {
    if (not tmp) {
        throw new Exception("temporary register set is empty");
    }

    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (uregset->at(target) != nullptr) {
        uregset->free(target);
    }
    uregset->set(target, tmp.release());

    return addr;
}
