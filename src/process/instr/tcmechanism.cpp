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
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
#include <viua/types/integer.h>
using namespace std;


auto viua::process::Process::optry(Op_address_type addr) -> Op_address_type {
    /** Create new special frame for try blocks.
     */
    if (stack->try_frame_new) {
        throw "new block frame requested while last one is unused";
    }
    stack->try_frame_new = make_unique<TryFrame>();
    return addr;
}

auto viua::process::Process::opcatch(Op_address_type addr) -> Op_address_type {
    /** Run catch instruction.
     */
    std::string type_name, catcher_block_name;
    tie(addr, type_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);
    tie(addr, catcher_block_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->is_block(catcher_block_name)) {
        throw make_unique<viua::types::Exception>(
            "registering undefined handler block '" + catcher_block_name
            + "' to handle " + type_name);
    }

    stack->try_frame_new->catchers[type_name] =
        make_unique<Catcher>(type_name, catcher_block_name);

    return addr;
}

auto viua::process::Process::opdraw(Op_address_type addr) -> Op_address_type {
    /** Run draw instruction.
     */
    if (viua::bytecode::decoder::operands::is_void(addr)) {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
        /*
         * Catching thrown objects into a void register will just discard them.
         */
        stack->caught.reset(nullptr);
    } else {
        viua::kernel::Register* target = nullptr;
        tie(addr, target) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);

        if (not stack->caught) {
            throw make_unique<viua::types::Exception>(
                "no caught object to draw");
        }
        *target = std::move(stack->caught);
    }

    return addr;
}

auto viua::process::Process::openter(Op_address_type addr) -> Op_address_type {
    /*  Run enter instruction.
     */
    auto block_name = std::string{};
    tie(addr, block_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->is_block(block_name)) {
        throw make_unique<viua::types::Exception>(
            "cannot enter undefined block: " + block_name);
    }

    auto block_address = adjust_jump_base_for_block(block_name);

    stack->try_frame_new->return_address   = addr;
    stack->try_frame_new->associated_frame = stack->back().get();
    stack->try_frame_new->block_name       = block_name;

    stack->tryframes.emplace_back(std::move(stack->try_frame_new));

    return block_address;
}

auto viua::process::Process::opthrow(Op_address_type addr) -> Op_address_type {
    /** Run throw instruction.
     */
    viua::kernel::Register* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    if (source->empty()) {
        ostringstream oss;
        oss << "throw from null register";
        throw make_unique<viua::types::Exception>(oss.str());
    }
    stack->thrown = source->give();

    return addr;
}

auto viua::process::Process::opleave(Op_address_type addr) -> Op_address_type {
    /*  Run leave instruction.
     */
    if (stack->tryframes.size() == 0) {
        throw make_unique<viua::types::Exception>(
            "bad leave: no block has been entered");
    }
    addr = stack->tryframes.back()->return_address;
    stack->tryframes.pop_back();

    if (stack->size() > 0) {
        adjust_jump_base_for(stack->back()->function_name);
    }
    return addr;
}
