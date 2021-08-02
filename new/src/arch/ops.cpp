/*
 *  Copyright (C) 2021 Marek Marecki
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

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>

#include <string>


namespace viua::arch::ops {
    auto to_string(opcode_type const raw) -> std::string
    {
        auto const greedy = std::string{
            static_cast<bool>(raw & GREEDY) ? "g." : ""
        };
        auto const opcode = static_cast<OPCODE>(raw & OPCODE_MASK);

        switch (opcode) {
            case OPCODE::NOOP:
                return greedy + "noop";
            case OPCODE::HALT:
                return greedy + "halt";
            case OPCODE::EBREAK:
                return greedy + "ebreak";
            case OPCODE::ADD:
                return greedy + "add";
            case OPCODE::SUB:
                return greedy + "sub";
            case OPCODE::MUL:
                return greedy + "mul";
            case OPCODE::DIV:
                return greedy + "div";
            case OPCODE::DELETE:
                return greedy + "delete";
            case OPCODE::LUI:
                return greedy + "lui";
            case OPCODE::LUIU:
                return greedy + "luiu";
            case OPCODE::ADDI:
                return greedy + "addi";
            case OPCODE::ADDIU:
                return greedy + "addiu";
        }

        return "<unknown>";
    }
}
