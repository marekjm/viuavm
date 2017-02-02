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
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/assert.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/vector.h>
#include <viua/types/pointer.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/kernel.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opvec(viua::internals::types::byte* addr) {
    viua::internals::types::register_index register_index = 0, pack_start_index = 0, pack_length = 0;
    tie(addr, register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, pack_start_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, pack_length) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if ((register_index > pack_start_index) and (register_index < (pack_start_index+pack_length))) {
        // FIXME vector is inserted into a register after packing, so this exception is not entirely well thought-out
        // allow packing target register
        throw new viua::types::Exception("vec would pack itself");
    }
    if ((pack_start_index+pack_length) >= currently_used_register_set->size()) {
        throw new viua::types::Exception("vec: packing outside of register set range");
    }
    for (decltype(pack_length) i = 0; i < pack_length; ++i) {
        if (currently_used_register_set->at(pack_start_index+i) == nullptr) {
            throw new viua::types::Exception("vec: cannot pack null register");
        }
    }

    unique_ptr<viua::types::Vector> v(new viua::types::Vector());
    for (decltype(pack_length) i = 0; i < pack_length; ++i) {
        v->push(pop(pack_start_index+i));
    }

    place(register_index, std::move(v));

    return addr;
}

viua::internals::types::byte* viua::process::Process::opvinsert(viua::internals::types::byte* addr) {
    viua::types::Type* vector_operand = nullptr;
    tie(addr, vector_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    viua::assertions::assert_implements<viua::types::Vector>(vector_operand, "viua::types::Vector");

    unique_ptr<viua::types::Type> object;
    if (viua::bytecode::decoder::operands::get_operand_type(addr) == OT_POINTER) {
        viua::types::Type* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);
        object = source->copy();
    } else {
        viua::kernel::Register* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);
        object = source->give();
    }

    viua::internals::types::register_index position_operand_index = 0;
    tie(addr, position_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    static_cast<viua::types::Vector*>(vector_operand)->insert(position_operand_index, std::move(object));

    return addr;
}

viua::internals::types::byte* viua::process::Process::opvpush(viua::internals::types::byte* addr) {
    viua::types::Type* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    viua::assertions::assert_implements<viua::types::Vector>(target, "viua::types::Vector");

    unique_ptr<viua::types::Type> object;
    if (viua::bytecode::decoder::operands::get_operand_type(addr) == OT_POINTER) {
        viua::types::Type* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);
        object = source->copy();
    } else {
        viua::kernel::Register* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);
        object = source->give();
    }

    static_cast<viua::types::Vector*>(target)->push(std::move(object));

    return addr;
}

viua::internals::types::byte* viua::process::Process::opvpop(viua::internals::types::byte* addr) {
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Type* vector_operand = nullptr;
    tie(addr, vector_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    viua::assertions::assert_implements<viua::types::Vector>(vector_operand, "viua::types::Vector");

    int position_operand_index = 0;
    tie(addr, position_operand_index) = viua::bytecode::decoder::operands::fetch_primitive_int(addr, this);

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    unique_ptr<viua::types::Type> ptr = static_cast<viua::types::Vector*>(vector_operand)->pop(position_operand_index);
    if (not void_target) {
        *target = std::move(ptr);
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opvat(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    viua::types::Type* vector_operand = nullptr;
    int position_operand_index = 0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    tie(addr, vector_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, position_operand_index) = viua::bytecode::decoder::operands::fetch_primitive_int(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(vector_operand, "viua::types::Vector");
    *target = static_cast<viua::types::Vector*>(vector_operand)->at(position_operand_index)->pointer();

    return addr;
}

viua::internals::types::byte* viua::process::Process::opvlen(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    viua::types::Type* source = nullptr;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(source, "viua::types::Vector");
    *target = unique_ptr<viua::types::Type>{new viua::types::Integer(static_cast<viua::types::Vector*>(source)->len())};

    return addr;
}
