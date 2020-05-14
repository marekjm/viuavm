/*
 *  Copyright (C) 2020 Marek Marecki
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
#include <viua/assembler/frontend/static_analyser.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_exception_value(Register_usage_profile& register_usage_profile,
                           Instruction const& instruction) -> void
{
    auto target = get_operand<Register_index>(instruction, 0);
    if (not target) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    auto ex = get_operand<Register_index>(instruction, 1);
    if (not ex) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_use_of_register(register_usage_profile, *ex);
    assert_type_of_register<viua::internals::Value_types::EXCEPTION>(
        register_usage_profile, *ex);

    auto val         = Register{};
    val.index        = target->index;
    val.register_set = target->rss;
    val.value_type   = viua::internals::Value_types::UNDEFINED;
    register_usage_profile.define(
        val, target->tokens.at(0), target->tokens.at(1));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
