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
                    auto check_op_swap(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        using viua::cg::lex::InvalidSyntax;

                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *target, "swap with");
                        if (target->as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE) {
                            throw InvalidSyntax(target->tokens.at(0), "invalid access mode")
                                .note("can only swap using direct access mode")
                                .aside(target->tokens.at(0),
                                       "did you mean '%" + target->tokens.at(0).str().substr(1) + "'?");
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source, "swap with");
                        if (source->as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE) {
                            throw InvalidSyntax(source->tokens.at(0), "invalid access mode")
                                .note("can only swap using direct access mode")
                                .aside(source->tokens.at(0),
                                       "did you mean '%" + source->tokens.at(0).str().substr(1) + "'?");
                        }

                        auto val_target = Register(*target);
                        val_target.value_type = register_usage_profile.at(*source).second.value_type;

                        auto val_source = Register(*source);
                        val_source.value_type = register_usage_profile.at(*target).second.value_type;

                        register_usage_profile.define(val_target, target->tokens.at(0));
                        register_usage_profile.define(val_source, source->tokens.at(0));
                    }
                }  // namespace checkers
            }                          // namespace static_analyser
        }                              // namespace frontend
    }                                  // namespace assembler
}  // namespace viua
