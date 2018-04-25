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
auto check_op_text(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void {
    auto operand = get_operand<RegisterIndex>(instruction, 0);
    if (not operand) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_if_name_resolved(register_usage_profile, *operand);

    auto source = get_operand<RegisterIndex>(instruction, 1);
    if (source) {
        check_use_of_register(register_usage_profile, *source);
        check_if_name_resolved(register_usage_profile, *source);

        auto const converted_from_type =
            register_usage_profile.at(Register(*source)).second.value_type;
        if (converted_from_type == viua::internals::ValueTypes::TEXT) {
            using viua::cg::lex::InvalidSyntax;
            using viua::cg::lex::TracedSyntaxError;
            throw TracedSyntaxError{}
                .append(invalid_syntax(instruction.operands.at(1)->tokens,
                                       "useless conversion of value"))
                .append(InvalidSyntax{
                    register_usage_profile.defined_where(Register(*source)), ""}
                            .note("value defined here"));
        }
    }

    auto val         = Register{};
    val.index        = operand->index;
    val.register_set = operand->rss;
    val.value_type   = viua::internals::ValueTypes::TEXT;

    register_usage_profile.define(val, operand->tokens.at(0));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
