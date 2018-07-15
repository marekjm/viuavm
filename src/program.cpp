/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/cg/tools.h>
#include <viua/program.h>
#include <viua/support/string.h>
#include <viua/util/memory.h>
using namespace std;

using viua::util::memory::aligned_write;
using viua::util::memory::load_aligned;


auto Program::bytecode() const
    -> std::unique_ptr<viua::internals::types::byte[]> {
    /*  Returns pointer to a copy of the bytecode.
     *  Each call produces new copy.
     *
     *  Calling code is responsible for proper destruction of the allocated
     * memory.
     */
    auto tmp = make_unique<viua::internals::types::byte[]>(bytes);
    for (decltype(bytes) i = 0; i < bytes; ++i) {
        tmp[i] = program[i];
    }
    return tmp;
}

auto Program::fill(std::unique_ptr<viua::internals::types::byte[]> code) -> Program& {
    program  = std::move(code);
    addr_ptr = program.get();
    return (*this);
}


auto Program::setdebug(bool d) -> Program& {
    debug = d;
    return (*this);
}

auto Program::setscream(bool d) -> Program& {
    scream = d;
    return (*this);
}


auto Program::size() const -> uint64_t {
    return bytes;
}

using Token = viua::cg::lex::Token;
auto Program::calculate_jumps(
    std::vector<tuple<uint64_t, uint64_t>> const jump_positions,
    std::vector<Token> const& tokens) -> Program& {

    uint64_t position, offset;
    uint64_t adjustment;
    for (auto jmp : jump_positions) {
        tie(position, offset) = jmp;

        // usually beware of the reinterpret_cast<>'s but here we *know* what
        // we're doing we *know* that this location points to uint64_t even if
        // it is stored inside the viua::internals::types::byte array
        ptr = reinterpret_cast<uint64_t*>(program.get() + position);
        adjustment =
            viua::cg::tools::calculate_bytecode_size_of_first_n_instructions2(
                tokens, load_aligned<uint64_t>(ptr));
        aligned_write(ptr) = (offset + adjustment);
    }

    return (*this);
}

auto Program::jumps() const -> std::vector<uint64_t> {
    auto jmps = std::vector<uint64_t>{};
    for (auto const jmp : branches) {
        jmps.push_back(static_cast<uint64_t>(jmp - program.get()));
    }
    return jmps;
}

Program::Program(viua::internals::types::bytecode_size const bts) : bytes(bts), debug(false), scream(false) {
    program = std::make_unique<viua::internals::types::byte[]>(bytes);
    /* Filling bytecode with zeroes (which are interpreted by kernel as NOP
     * instructions) is a safe way to prevent many hiccups.
     */
    std::fill_n(program.get(), bytes, viua::internals::types::byte{0});
    addr_ptr = program.get();
}
Program::Program(Program const& that)
        : program(nullptr), bytes(that.bytes), addr_ptr(nullptr), branches({}) {
    program = std::make_unique<viua::internals::types::byte[]>(bytes);
    std::copy_n(program.get(), bytes, that.program.get());
    addr_ptr = program.get() + (that.addr_ptr - that.program.get());

    for (unsigned i = 0; i < that.branches.size(); ++i) {
        branches.push_back(program.get()
                           + (that.branches[i] - that.program.get()));
    }
}
