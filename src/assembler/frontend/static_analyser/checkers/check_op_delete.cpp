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
                    auto check_op_delete(Register_usage_profile& register_usage_profile,
                                         Instruction const& instruction) -> void {
                        using viua::cg::lex::InvalidSyntax;

                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *target, "delete of");
                        if (target->as != viua::internals::AccessSpecifier::DIRECT) {
                            throw InvalidSyntax(target->tokens.at(0), "invalid access mode")
                                .note("can only delete using direct access mode")
                                .aside("did you mean '%" + target->tokens.at(0).str().substr(1) + "'?");
                        }
                        register_usage_profile.erase(Register(*target), instruction.tokens.at(0));
                    }
                }  // namespace checkers
            }      // namespace static_analyser
        }          // namespace frontend
    }              // namespace assembler
}  // namespace viua
