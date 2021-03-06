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

#include <endian.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>

#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/cg/tools.h>
#include <viua/program.h>
#include <viua/support/string.h>
#include <viua/util/memory.h>

using viua::util::memory::aligned_write;
using viua::util::memory::load_aligned;


auto Program::bytecode() const -> std::unique_ptr<uint8_t[]>
{
    auto tmp = std::make_unique<uint8_t[]>(bytes);
    std::copy_n(program.get(), size(), tmp.get());
    return tmp;
}

auto Program::fill(std::unique_ptr<uint8_t[]> code) -> Program&
{
    program  = std::move(code);
    addr_ptr = program.get();
    return (*this);
}


auto Program::size() const -> uint64_t
{
    return bytes;
}

using Token = viua::cg::lex::Token;
auto Program::calculate_jumps(
    std::vector<std::tuple<uint64_t, uint64_t>> const jump_positions,
    std::vector<Token> const& tokens) -> Program&
{
    for (auto const& jmp : jump_positions) {
        auto const [position, offset] = jmp;

        /*
         * Usually beware of the reinterpret_cast<> but here we *know* what we
         * are doing. We *know* that this location points to uint64_t even if it
         * is stored inside the uint8_t array.
         */
        auto const ptr            = (program.get() + position);
        auto const n_instructions = be64toh(load_aligned<uint64_t>(ptr));
        auto const adjustment =
            viua::cg::tools::calculate_bytecode_size_of_first_n_instructions(
                tokens, n_instructions);
        aligned_write(ptr) = htobe64(offset + adjustment);
    }

    return (*this);
}

auto Program::jumps() const -> std::vector<uint64_t>
{
    auto jmps = std::vector<uint64_t>{};
    for (auto const jmp : branches) {
        jmps.push_back(static_cast<uint64_t>(jmp - program.get()));
    }
    return jmps;
}

Program::Program(viua::bytecode::codec::bytecode_size_type const bts)
        : bytes{bts}
{
    program = std::make_unique<uint8_t[]>(bytes);

    /*
     * Filling bytecode with zeroes (which are interpreted by kernel as NOP
     * instructions) is a safe way to prevent many hiccups.
     */
    std::fill_n(program.get(), bytes, uint8_t{0});
    addr_ptr = program.get();
}
Program::Program(Program const& that)
        : program(nullptr), bytes(that.bytes), addr_ptr(nullptr), branches({})
{
    program = std::make_unique<uint8_t[]>(bytes);
    std::copy_n(program.get(), bytes, that.program.get());
    addr_ptr = program.get() + (that.addr_ptr - that.program.get());

    for (unsigned i = 0; i < that.branches.size(); ++i) {
        branches.push_back(program.get()
                           + (that.branches[i] - that.program.get()));
    }
}
