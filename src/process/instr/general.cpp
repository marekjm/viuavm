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

#include <cstdint>
#include <iostream>
#include <memory>
#include <viua/util/memory.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>


auto viua::process::Process::opecho(Op_address_type addr) -> Op_address_type {
    auto source = viua::util::memory::dumb_ptr<viua::types::Value>{nullptr};
    std::tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object(addr, this);
    std::cout << source->str();
    return addr;
}

auto viua::process::Process::opprint(Op_address_type addr) -> Op_address_type {
    auto source = viua::util::memory::dumb_ptr<viua::types::Value>{nullptr};
    std::tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object(addr, this);
    std::cout << source->str() + '\n';
    return addr;
}


auto viua::process::Process::opjump(Op_address_type addr) -> Op_address_type {
    auto target =
        Op_address_type{ stack->jump_base
            + viua::bytecode::decoder::operands::extract_primitive_uint64(addr,
                    this) };
    if (target == addr) {
        throw std::make_unique<viua::types::Exception>(
            "aborting: JUMP instruction pointing to itself");
    }
    return target;
}

auto viua::process::Process::opif(Op_address_type addr) -> Op_address_type {
    auto source = viua::util::memory::dumb_ptr<viua::types::Value>{nullptr};
    std::tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object(addr, this);

    auto addr_true = viua::internals::types::bytecode_size{0};
    auto addr_false = viua::internals::types::bytecode_size{0};
    std::tie(addr, addr_true) =
        viua::bytecode::decoder::operands::fetch_primitive_uint64(addr, this);
    std::tie(addr, addr_false) =
        viua::bytecode::decoder::operands::fetch_primitive_uint64(addr, this);

    return (stack->jump_base + (source->boolean() ? addr_true : addr_false));
}
