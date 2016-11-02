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
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* viua::process::Process::opmove(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    uregset->move(source, target);

    return addr;
}
byte* viua::process::Process::opcopy(byte* addr) {
    unsigned target = 0;
    viua::types::Type *source = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    place(target, source->copy());

    return addr;
}
byte* viua::process::Process::opptr(byte* addr) {
    unsigned target = 0;
    viua::types::Type *source = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    place(target, source->pointer());

    return addr;
}
byte* viua::process::Process::opswap(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    uregset->swap(target, source);

    return addr;
}
byte* viua::process::Process::opdelete(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    uregset->free(target);

    return addr;
}
byte* viua::process::Process::opisnull(byte* addr) {
    unsigned target = 0, source = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    place(target, new viua::types::Boolean(uregset->at(source) == nullptr));

    return addr;
}

byte* viua::process::Process::opress(byte* addr) {
    /*  Run ress instruction.
     */
    int to_register_set = 0;
    tie(addr, to_register_set) = viua::bytecode::decoder::operands::fetch_raw_int(addr, this);

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
            throw new viua::types::Exception("illegal register set ID in ress instruction");
    }

    return addr;
}

byte* viua::process::Process::optmpri(byte* addr) {
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    tmp.reset(pop(target));
    return addr;
}
byte* viua::process::Process::optmpro(byte* addr) {
    if (not tmp) {
        throw new viua::types::Exception("temporary register set is empty");
    }

    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (uregset->at(target) != nullptr) {
        uregset->free(target);
    }
    uregset->set(target, tmp.release());

    return addr;
}
