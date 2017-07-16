/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
#include <viua/types/bits.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
#include <viua/util/memory.h>
using namespace std;

using viua::util::memory::load_aligned;


viua::internals::types::byte* viua::process::Process::opbits(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_BITS) {
        ++addr;  // for operand type
        auto bits_size = load_aligned<viua::internals::types::bits_size>(addr);
        addr += sizeof(bits_size);
        *target = make_unique<viua::types::Bits>(bits_size, addr);
        addr += bits_size;
    } else {
        viua::types::Integer* n = nullptr;
        tie(addr, n) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);
        *target = unique_ptr<viua::types::Value>{new viua::types::Bits(n->as_unsigned())};
    }

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitand(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Bits* lhs = nullptr;
    tie(addr, lhs) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Bits* rhs = nullptr;
    tie(addr, rhs) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    *target = (*lhs) & (*rhs);

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitor(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Bits* lhs = nullptr;
    tie(addr, lhs) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Bits* rhs = nullptr;
    tie(addr, rhs) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    *target = (*lhs) | (*rhs);

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitnot(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Bits* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    *target = source->inverted();

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitxor(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Bits* lhs = nullptr;
    tie(addr, lhs) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Bits* rhs = nullptr;
    tie(addr, rhs) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    *target = (*lhs) ^ (*rhs);

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitat(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Bits* bits = nullptr;
    tie(addr, bits) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Integer* n = nullptr;
    tie(addr, n) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    *target = unique_ptr<viua::types::Value>{new viua::types::Boolean(bits->at(n->as_unsigned()))};

    return addr;
}


viua::internals::types::byte* viua::process::Process::opbitset(viua::internals::types::byte* addr) {
    viua::types::Bits* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Integer* index = nullptr;
    tie(addr, index) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    bool value = false;
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_TRUE) {
        ++addr;  // for operand type
        value = true;
    } else if (ot == OT_FALSE) {
        ++addr;  // for operand type
        value = false;
    } else {
        viua::types::Boolean* x = nullptr;
        tie(addr, x) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Boolean>(addr, this);
        value = x->boolean();
    }

    target->set(index->as_unsigned(), value);

    return addr;
}

using BitShiftOp = decltype(&viua::types::Bits::shl);
template<const BitShiftOp op>
static auto execute_bit_shift_instruction(viua::process::Process* process, viua::internals::types::byte* addr)
    -> viua::internals::types::byte* {
    viua::kernel::Register* target = nullptr;
    if (viua::bytecode::decoder::operands::is_void(addr)) {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    } else {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, process);
    }

    viua::types::Bits* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, process);

    viua::types::Integer* offset = nullptr;
    tie(addr, offset) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, process);

    auto result = (source->*op)(offset->as_unsigned());
    if (target) {
        *target = std::move(result);
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opshl(viua::internals::types::byte* addr) {
    return execute_bit_shift_instruction<&viua::types::Bits::shl>(this, addr);
}


viua::internals::types::byte* viua::process::Process::opshr(viua::internals::types::byte* addr) {
    return execute_bit_shift_instruction<&viua::types::Bits::shr>(this, addr);
}


viua::internals::types::byte* viua::process::Process::opashl(viua::internals::types::byte* addr) {
    return execute_bit_shift_instruction<&viua::types::Bits::ashl>(this, addr);
}


viua::internals::types::byte* viua::process::Process::opashr(viua::internals::types::byte* addr) {
    return execute_bit_shift_instruction<&viua::types::Bits::ashr>(this, addr);
}


viua::internals::types::byte* viua::process::Process::oprol(viua::internals::types::byte* addr) {
    viua::types::Bits* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Integer* offset = nullptr;
    tie(addr, offset) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    target->rol(offset->as_unsigned());

    return addr;
}


viua::internals::types::byte* viua::process::Process::opror(viua::internals::types::byte* addr) {
    viua::types::Bits* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Bits>(addr, this);

    viua::types::Integer* offset = nullptr;
    tie(addr, offset) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    target->ror(offset->as_unsigned());

    return addr;
}
