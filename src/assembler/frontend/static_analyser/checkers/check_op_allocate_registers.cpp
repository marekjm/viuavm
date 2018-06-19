/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <limits>
#include <string>
#include <viua/assembler/frontend/static_analyser.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_allocate_registers(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void {
    auto const operand = get_operand<Register_index>(instruction, 0);

    auto const max_register_index = std::numeric_limits<viua::internals::types::register_index>::max();
    if (operand->index >= max_register_index) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                "cannot allocate more than " + std::to_string(max_register_index) + " local registers");
    }

    register_usage_profile.allocated_registers(operand->index);
    register_usage_profile.allocated_where(operand->tokens.at(0));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers