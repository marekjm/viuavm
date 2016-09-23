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
#include <viua/types/integer.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::optry(byte* addr) {
    /** Create new special frame for try blocks.
     */
    if (try_frame_new) {
        throw "new block frame requested while last one is unused";
    }
    try_frame_new.reset(new TryFrame());
    return addr;
}

byte* Process::opcatch(byte* addr) {
    /** Run catch instruction.
     */
    string type_name, catcher_block_name;
    tie(addr, type_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);
    tie(addr, catcher_block_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isBlock(catcher_block_name)) {
        throw new Exception("registering undefined handler block '" + catcher_block_name + "' to handle " + type_name);
    }

    try_frame_new->catchers[type_name] = new Catcher(type_name, catcher_block_name);

    return addr;
}

byte* Process::oppull(byte* addr) {
    /** Run pull instruction.
     */
    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (not caught) {
        throw new Exception("no caught object to pull");
    }
    uregset->set(target, caught.release());

    return addr;
}

byte* Process::openter(byte* addr) {
    /*  Run enter instruction.
     */
    string block_name;
    tie(addr, block_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isBlock(block_name)) {
        throw new Exception("cannot enter undefined block: " + block_name);
    }

    byte* block_address = adjustJumpBaseForBlock(block_name);

    try_frame_new->return_address = addr;
    try_frame_new->associated_frame = frames.back().get();
    try_frame_new->block_name = block_name;

    tryframes.emplace_back(std::move(try_frame_new));

    return block_address;
}

byte* Process::opthrow(byte* addr) {
    /** Run throw instruction.
     */
    unsigned source = 0;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (source >= uregset->size()) {
        ostringstream oss;
        oss << "invalid read: register out of bounds: " << source;
        throw new Exception(oss.str());
    }
    if (uregset->at(source) == nullptr) {
        ostringstream oss;
        oss << "invalid throw: register " << source << " is empty";
        throw new Exception(oss.str());
    }

    thrown.reset(uregset->pop(source));

    return addr;
}

byte* Process::opleave(byte* addr) {
    /*  Run leave instruction.
     */
    if (tryframes.size() == 0) {
        throw new Exception("bad leave: no block has been entered");
    }
    addr = tryframes.back()->return_address;
    tryframes.pop_back();

    if (frames.size() > 0) {
        adjustJumpBaseFor(frames.back()->function_name);
    }
    return addr;
}
