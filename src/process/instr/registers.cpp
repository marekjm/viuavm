/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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


viua::internals::types::byte* viua::process::Process::opmove(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr, *source = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = std::move(*source);

    return addr;
}
viua::internals::types::byte* viua::process::Process::opcopy(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = source->copy();

    return addr;
}
viua::internals::types::byte* viua::process::Process::opptr(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = source->pointer();

    return addr;
}
viua::internals::types::byte* viua::process::Process::opswap(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr, *source = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    target->swap(*source);

    return addr;
}
viua::internals::types::byte* viua::process::Process::opdelete(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    if (target->empty()) {
        throw new viua::types::Exception("delete of null register");
    }
    target->give();

    return addr;
}
viua::internals::types::byte* viua::process::Process::opisnull(viua::internals::types::byte* addr) {
    viua::kernel::Register *target = nullptr, *source = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = unique_ptr<viua::types::Type>{new viua::types::Boolean(source->empty())};

    return addr;
}

viua::internals::types::byte* viua::process::Process::opress(viua::internals::types::byte* addr) {
    viua::internals::types::registerset_type_marker to_register_set = 0;
    tie(addr, to_register_set) = viua::bytecode::decoder::operands::fetch_registerset_type(addr, this);

    switch (static_cast<viua::internals::RegisterSets>(to_register_set)) {
        case viua::internals::RegisterSets::GLOBAL:
            currently_used_register_set = global_register_set.get();
            break;
        case viua::internals::RegisterSets::LOCAL:
            currently_used_register_set = stack.frames.back()->local_register_set.get();
            break;
        case viua::internals::RegisterSets::STATIC:
            ensureStaticRegisters(stack.frames.back()->function_name);
            currently_used_register_set = static_registers.at(stack.frames.back()->function_name).get();
            break;
        default:
            throw new viua::types::Exception("illegal register set ID in ress instruction");
    }

    return addr;
}
