/*
 *  Copyright (C) 2015, 2016, 2020 Marek Marecki
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
#include <iomanip>
#include <map>
#include <string>

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>


int main()
{
    for (auto i = uint8_t{0}; i < static_cast<uint8_t>(0xff); ++i) {
        auto const opcode = static_cast<OPCODE>(i);
        if (!OP_NAMES.count(opcode)) {
            continue;
        }

        std::cout
            << std::dec
            << std::setw(3)
            << std::setfill(' ')
            << static_cast<uint64_t>(opcode);
        std::cout << "  0x";
        std::cout
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<uint64_t>(opcode);

        std::cout << "  " << OP_NAMES.at(opcode) << '\n';
    }
    return 0;
}
