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
auto check_op_closure(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction,
                      std::map<Register, Closure>& created_closures) -> void {
    using viua::assembler::frontend::parser::FunctionNameLiteral;

    auto target = get_operand<RegisterIndex>(instruction, 0);
    if (not target) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_if_name_resolved(register_usage_profile, *target);

    auto fn = get_operand<FunctionNameLiteral>(instruction, 1);
    if (not fn) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected function name literal");
    }

    /*
     * FIXME The SA should switch to verification of the closure after it has
     * been created and all required values captured. The SA will need some
     * guidance to discover the precise moment at which it can start checking
     * the closure for correctness.
     */

    auto val       = Register{*target};
    val.value_type = ValueTypes::CLOSURE;
    register_usage_profile.define(val, target->tokens.at(0));

    created_closures[val] = Closure{fn->content};
}
}  // namespace checkers
}  // namespace static_analyser
}  // namespace frontend
}  // namespace assembler
}  // namespace viua
