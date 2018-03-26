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
#include <viua/assembler/frontend/static_analyser.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua {
namespace assembler {
namespace frontend {
namespace static_analyser {
namespace checkers {
auto check_op_isnull(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void {
    using viua::cg::lex::InvalidSyntax;
    using viua::cg::lex::TracedSyntaxError;

    auto target = get_operand<RegisterIndex>(instruction, 0);
    if (not target) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    auto source = get_operand<RegisterIndex>(instruction, 1);
    if (not source) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    if (source->as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE) {
        throw InvalidSyntax(source->tokens.at(0), "invalid access mode")
            .note("can only check using direct access mode")
            .aside("did you mean '%" + source->tokens.at(0).str().substr(1) +
                   "'?");
    }

    if (register_usage_profile.defined(Register{*source})) {
        throw TracedSyntaxError{}
            .append(
                InvalidSyntax{source->tokens.at(0),
                              "useless check, register will always be defined"})
            .append(InvalidSyntax{
                register_usage_profile.defined_where(Register{*source})}
                        .note("register is defined here"));
    }

    auto val = Register(*target);
    val.value_type = ValueTypes::BOOLEAN;
    register_usage_profile.define(val, target->tokens.at(0));
}
}  // namespace checkers
}  // namespace static_analyser
}  // namespace frontend
}  // namespace assembler
}  // namespace viua
