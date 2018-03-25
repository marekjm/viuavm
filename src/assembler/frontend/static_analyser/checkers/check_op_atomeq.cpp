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
                    auto check_op_atomeq(Register_usage_profile& register_usage_profile,
                                         Instruction const& instruction) -> void {
                        auto result = get_operand<RegisterIndex>(instruction, 0);
                        if (not result) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_if_name_resolved(register_usage_profile, *result);

                        auto lhs = get_operand<RegisterIndex>(instruction, 1);
                        if (not lhs) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens,
                                                 "invalid left-hand side operand")
                                .note("expected register index");
                        }

                        auto rhs = get_operand<RegisterIndex>(instruction, 2);
                        if (not rhs) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens,
                                                 "invalid right-hand side operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *lhs);
                        check_use_of_register(register_usage_profile, *rhs);

                        assert_type_of_register<ValueTypes::ATOM>(register_usage_profile, *lhs);
                        assert_type_of_register<ValueTypes::ATOM>(register_usage_profile, *rhs);

                        auto val = Register(*result);
                        val.value_type = viua::internals::ValueTypes::BOOLEAN;
                        register_usage_profile.define(val, result->tokens.at(0));
                    }
                }  // namespace checkers
            }      // namespace static_analyser
        }          // namespace frontend
    }              // namespace assembler
}  // namespace viua
