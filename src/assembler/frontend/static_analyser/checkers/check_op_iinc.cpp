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

#include <string>
#include <vector>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/support/string.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_iinc(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void
{
    using viua::assembler::frontend::parser::Register_index;
    auto operand = get_operand<Register_index>(instruction, 0);
    if (not operand) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_if_name_resolved(register_usage_profile, *operand);

    check_use_of_register(register_usage_profile, *operand);
    assert_type_of_register<viua::internals::Value_types::INTEGER>(
        register_usage_profile, *operand);
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
