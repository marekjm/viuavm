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
#include <viua/types/integer.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::optry(viua::internals::types::byte* addr) {
    /** Create new special frame for try blocks.
     */
    if (try_frame_new) {
        throw "new block frame requested while last one is unused";
    }
    try_frame_new.reset(new TryFrame());
    return addr;
}

viua::internals::types::byte* viua::process::Process::opcatch(viua::internals::types::byte* addr) {
    /** Run catch instruction.
     */
    string type_name, catcher_block_name;
    tie(addr, type_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);
    tie(addr, catcher_block_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isBlock(catcher_block_name)) {
        throw new viua::types::Exception("registering undefined handler block '" + catcher_block_name + "' to handle " + type_name);
    }

    try_frame_new->catchers[type_name] = new Catcher(type_name, catcher_block_name);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opdraw(viua::internals::types::byte* addr) {
    /** Run draw instruction.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    if (not stack.caught) {
        throw new viua::types::Exception("no caught object to draw");
    }
    *target = std::move(stack.caught);

    return addr;
}

viua::internals::types::byte* viua::process::Process::openter(viua::internals::types::byte* addr) {
    /*  Run enter instruction.
     */
    string block_name;
    tie(addr, block_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isBlock(block_name)) {
        throw new viua::types::Exception("cannot enter undefined block: " + block_name);
    }

    viua::internals::types::byte* block_address = adjustJumpBaseForBlock(block_name);

    try_frame_new->return_address = addr;
    try_frame_new->associated_frame = stack.frames.back().get();
    try_frame_new->block_name = block_name;

    tryframes.emplace_back(std::move(try_frame_new));

    return block_address;
}

viua::internals::types::byte* viua::process::Process::opthrow(viua::internals::types::byte* addr) {
    /** Run throw instruction.
     */
    viua::kernel::Register* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    if (source->empty()) {
        ostringstream oss;
        oss << "throw from null register";
        throw new viua::types::Exception(oss.str());
    }
    stack.thrown = source->give();

    return addr;
}

viua::internals::types::byte* viua::process::Process::opleave(viua::internals::types::byte* addr) {
    /*  Run leave instruction.
     */
    if (tryframes.size() == 0) {
        throw new viua::types::Exception("bad leave: no block has been entered");
    }
    addr = tryframes.back()->return_address;
    tryframes.pop_back();

    if (stack.frames.size() > 0) {
        adjustJumpBaseFor(stack.frames.back()->function_name);
    }
    return addr;
}
