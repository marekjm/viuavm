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

#include <algorithm>
#include <iostream>
#include <map>
#include <string>

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>


int main()
{
    auto const max_mnemonic_length = []() -> std::string::size_type {
        auto l = std::string::size_type{0};
        for (std::pair<const OPCODE, std::string> i : OP_NAMES) {
            l = std::max(l, i.second.size());
        }
        return (l + 1);
    }();

    auto const initial_column = std::string{"MNEMONIC            "};
    std::cout << initial_column << "| OPCODE  | HEX OPCODE\n" << std::endl;

    auto const column_length =
        std::max(max_mnemonic_length, initial_column.size());

    for (auto i = uint8_t{0}; i < static_cast<uint8_t>(0xff); ++i) {
        auto const opcode = static_cast<OPCODE>(i);
        auto mnemonic     = std::string{"??"};
        if (OP_NAMES.count(opcode)) {
            mnemonic = OP_NAMES.at(opcode);
        } else {
            continue;
        }

        std::cout << mnemonic;
        std::cout << ' ';
        for (auto j = mnemonic.size() + 1; j < column_length; ++j) {
            std::cout << '.';
        }
        std::cout << "| ";

        std::cout << "  ";
        std::cout << (opcode < 10 ? " " : "");
        std::cout << (opcode < 100 ? " " : "");
        std::cout << opcode;
        std::cout << "       ";
        std::cout << "0x";
        std::cout << (opcode < 0x10 ? "0" : "");
        std::cout << std::hex << opcode << std::dec;
        std::cout << '\n';
    }
    return 0;
}
