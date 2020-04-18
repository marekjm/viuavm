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

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_atom(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void
{
    using viua::assembler::frontend::parser::Atom_literal;
    using viua::assembler::frontend::parser::Text_literal;

    auto operand = get_operand<Register_index>(instruction, 0);
    if (not operand) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_if_name_resolved(register_usage_profile, *operand);

    auto source = get_operand<Atom_literal>(instruction, 1);
    if (not source) {
        auto error = invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected atom literal");
        if (auto s = get_operand<Text_literal>(instruction, 1); s) {
            error.aside(
                instruction.operands.at(1)->tokens.at(0)
                , ("atom literals use single quotes: '"
                   + s->content.substr(1, s->content.size() - 2) + "'"));
        }
        throw error;
    }

    auto val         = Register{};
    val.index        = operand->index;
    val.register_set = operand->rss;
    val.value_type   = Value_types::ATOM;

    register_usage_profile.define(val, operand->tokens.at(0));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
