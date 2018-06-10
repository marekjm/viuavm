/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
#include <viua/assert.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/kernel/kernel.h>
#include <viua/kernel/registerset.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
using namespace std;

using viua::bytecode::decoder::operands::fetch_and_advance_addr;
using viua::bytecode::decoder::operands::fetch_optional_and_advance_addr;
using Register_index = viua::internals::types::register_index;

auto viua::process::Process::opvector(Op_address_type addr) -> Op_address_type {
    auto const [ target_rs, target_ri ] = fetch_and_advance_addr<viua::internals::RegisterSets, Register_index>(
        viua::bytecode::decoder::operands::fetch_register_type_and_index, addr, this);

    auto const [ pack_start_rs, pack_start_ri ] = fetch_and_advance_addr<viua::internals::RegisterSets, Register_index>(
        viua::bytecode::decoder::operands::fetch_register_type_and_index, addr, this);

    auto const pack_size = fetch_and_advance_addr<Register_index>(
        viua::bytecode::decoder::operands::fetch_register_index, addr, this);

    if ((target_ri > pack_start_ri)
        and (target_ri < (pack_start_ri + pack_size))) {
        // FIXME vector is inserted into a register after packing, so this
        // exception is not entirely well thought-out allow packing target
        // register
        throw make_unique<viua::types::Exception>("vector would pack itself");
    }
    if ((pack_start_ri + pack_size) >= currently_used_register_set->size()) {
        throw make_unique<viua::types::Exception>(
            "vector: packing outside of register set range");
    }
    for (auto i = decltype(pack_size){0}; i < pack_size; ++i) {
        if (register_at(pack_start_ri + i, pack_start_rs)->empty()) {
            throw make_unique<viua::types::Exception>(
                "vector: cannot pack null register");
        }
    }

    auto v = make_unique<viua::types::Vector>();
    for (auto i = decltype(pack_size){0}; i < pack_size; ++i) {
        v->push(register_at(pack_start_ri + i, pack_start_rs)->give());
    }

    *register_at(target_ri, target_rs) = std::move(v);

    return addr;
}

auto viua::process::Process::opvinsert(Op_address_type addr)
    -> Op_address_type {
    auto const vector_operand = fetch_and_advance_addr<viua::types::Vector*>(
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Vector>,
        addr,
        this);

    auto object = std::unique_ptr<viua::types::Value>{};
    if (viua::bytecode::decoder::operands::get_operand_type(addr)
        == OT_POINTER) {
        auto const source = fetch_and_advance_addr<viua::types::Value*>(
            viua::bytecode::decoder::operands::fetch_object, addr, this);
        object = source->copy();
    } else {
        auto const source = fetch_and_advance_addr<viua::kernel::Register*>(
            viua::bytecode::decoder::operands::fetch_register, addr, this);
        object = source->give();
    }

    viua::types::Integer* index_operand = nullptr;
    int64_t position_operand_index      = 0;

    if (not viua::bytecode::decoder::operands::is_void(addr)) {
        tie(addr, index_operand) =
            viua::bytecode::decoder::operands::fetch_object_of<
                viua::types::Integer>(addr, this);
        position_operand_index = index_operand->as_integer();
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    vector_operand->insert(position_operand_index, std::move(object));

    return addr;
}

auto viua::process::Process::opvpush(Op_address_type addr) -> Op_address_type {
    viua::types::Vector* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Vector>(
            addr, this);

    std::unique_ptr<viua::types::Value> object;
    if (viua::bytecode::decoder::operands::get_operand_type(addr)
        == OT_POINTER) {
        viua::types::Value* source = nullptr;
        tie(addr, source) =
            viua::bytecode::decoder::operands::fetch_object(addr, this);
        object = source->copy();
    } else {
        viua::kernel::Register* source = nullptr;
        tie(addr, source) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
        object = source->give();
    }

    target->push(std::move(object));

    return addr;
}

auto viua::process::Process::opvpop(Op_address_type addr) -> Op_address_type {
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        tie(addr, target) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Vector* vector_operand = nullptr;
    tie(addr, vector_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Vector>(
            addr, this);

    viua::types::Integer* index_operand = nullptr;
    int64_t position_operand_index      = -1;

    if (not viua::bytecode::decoder::operands::is_void(addr)) {
        tie(addr, index_operand) =
            viua::bytecode::decoder::operands::fetch_object_of<
                viua::types::Integer>(addr, this);
        position_operand_index = index_operand->as_integer();
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    std::unique_ptr<viua::types::Value> ptr =
        vector_operand->pop(position_operand_index);
    if (not void_target) {
        *target = std::move(ptr);
    }

    return addr;
}

auto viua::process::Process::opvat(Op_address_type addr) -> Op_address_type {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Vector* vector_operand = nullptr;
    tie(addr, vector_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Vector>(
            addr, this);

    viua::types::Integer* index_operand = nullptr;
    tie(addr, index_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<
            viua::types::Integer>(addr, this);

    *target = vector_operand->at(index_operand->as_integer())->pointer(this);

    return addr;
}

auto viua::process::Process::opvlen(Op_address_type addr) -> Op_address_type {
    viua::kernel::Register* target = nullptr;
    viua::types::Vector* source    = nullptr;

    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Vector>(
            addr, this);

    *target = make_unique<viua::types::Integer>(source->len());

    return addr;
}
