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
#include <viua/support/string.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua {
    namespace assembler {
        namespace frontend {
            namespace static_analyser {
                namespace checkers {
                    auto check_op_if(Register_usage_profile& register_usage_profile, ParsedSource const& ps,
                                     Instruction const& instruction, InstructionsBlock const& ib,
                                     InstructionIndex i, InstructionIndex const mnemonic_counter) -> void {
                        using viua::assembler::frontend::parser::Label;
                        using viua::assembler::frontend::parser::Offset;
                        using viua::cg::lex::InvalidSyntax;
                        using viua::cg::lex::TracedSyntaxError;

                        auto source = get_operand<RegisterIndex>(instruction, 0);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }
                        check_use_of_register(register_usage_profile, *source, "branch depends on");
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto jump_target_if_true = InstructionIndex{0};
                        if (auto offset = get_operand<Offset>(instruction, 1); offset) {
                            auto const jump_target = std::stol(offset->tokens.at(0));
                            if (jump_target > 0) {
                                jump_target_if_true = get_line_index_of_instruction(
                                    mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (auto label = get_operand<Label>(instruction, 1); label) {
                            auto jump_target =
                                get_line_index_of_instruction(ib.marker_map.at(label->tokens.at(0)), ib);
                            if (jump_target > i) {
                                jump_target_if_true = jump_target;
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (str::ishex(instruction.operands.at(1)->tokens.at(0))) {
                            // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump
                            // instructions. Do not check them now, but this should be fixed in the future.
                            // FIXME Return now and abort further checking of this block or risk throwing
                            // *many* false positives.
                            return;
                        } else {
                            throw InvalidSyntax(instruction.operands.at(1)->tokens.at(0),
                                                "invalid operand for if instruction");
                        }

                        auto jump_target_if_false = InstructionIndex{0};
                        if (auto offset = get_operand<Offset>(instruction, 2); offset) {
                            auto const jump_target = std::stol(offset->tokens.at(0));
                            if (jump_target > 0) {
                                jump_target_if_false = get_line_index_of_instruction(
                                    mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (auto label = get_operand<Label>(instruction, 2); label) {
                            auto jump_target =
                                get_line_index_of_instruction(ib.marker_map.at(label->tokens.at(0)), ib);
                            if (jump_target > i) {
                                jump_target_if_false = jump_target;
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (str::ishex(instruction.operands.at(2)->tokens.at(0))) {
                            // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump
                            // instructions. Do not check them now, but this should be fixed in the future.
                            // FIXME Return now and abort further checking of this block or risk throwing
                            // *many* false positives.
                            return;
                        } else {
                            throw InvalidSyntax(instruction.operands.at(2)->tokens.at(0),
                                                "invalid operand for if instruction");
                        }

                        auto register_with_unused_value = std::string{};

                        try {
                            Register_usage_profile register_usage_profile_if_true = register_usage_profile;
                            register_usage_profile_if_true.defresh();
                            check_register_usage_for_instruction_block_impl(register_usage_profile_if_true,
                                                                            ps, ib, jump_target_if_true,
                                                                            mnemonic_counter);
                        } catch (viua::cg::lex::UnusedValue& e) {
                            // Do not fail yet, because the value may be used by false branch.
                            // Save the error for later rethrowing.
                            register_with_unused_value = e.what();
                        } catch (InvalidSyntax& e) {
                            throw TracedSyntaxError{}.append(e).append(
                                InvalidSyntax{instruction.tokens.at(0), "after taking true branch here:"}.add(
                                    instruction.operands.at(1)->tokens.at(0)));
                        } catch (TracedSyntaxError& e) {
                            throw e.append(
                                InvalidSyntax{instruction.tokens.at(0), "after taking true branch here:"}.add(
                                    instruction.operands.at(1)->tokens.at(0)));
                        }

                        try {
                            Register_usage_profile register_usage_profile_if_false = register_usage_profile;
                            register_usage_profile_if_false.defresh();
                            check_register_usage_for_instruction_block_impl(register_usage_profile_if_false,
                                                                            ps, ib, jump_target_if_false,
                                                                            mnemonic_counter);
                        } catch (viua::cg::lex::UnusedValue& e) {
                            if (register_with_unused_value == e.what()) {
                                throw TracedSyntaxError{}.append(e).append(
                                    InvalidSyntax{instruction.tokens.at(0), "after taking either branch:"});
                            } else {
                                /*
                                 * If an error was thrown for a different register it means that the register
                                 * that was unused in true branch was used in the false one (so no errror),
                                 * and the register for which the false branch threw was used in the true one
                                 * (so no error either).
                                 */
                            }
                        } catch (InvalidSyntax& e) {
                            if (register_with_unused_value != e.what() and
                                std::string{e.what()}.substr(0, 6) == "unused") {
                                /*
                                 * If an error was thrown for a different register it means that the register
                                 * that was unused in true branch was used in the false one (so no errror),
                                 * and the register for which the false branch threw was used in the true one
                                 * (so no error either).
                                 */
                            } else {
                                throw TracedSyntaxError{}.append(e).append(
                                    InvalidSyntax{instruction.tokens.at(0), "after taking false branch here:"}
                                        .add(instruction.operands.at(2)->tokens.at(0)));
                            }
                        } catch (TracedSyntaxError& e) {
                            if (register_with_unused_value != e.errors.front().what() and
                                std::string{e.errors.front().what()}.substr(0, 6) == "unused") {
                                /*
                                 * If an error was thrown for a different register it means that the register
                                 * that was unused in true branch was used in the false one (so no errror),
                                 * and the register for which the false branch threw was used in the true one
                                 * (so no error either).
                                 */
                            } else {
                                throw e.append(
                                    InvalidSyntax{instruction.tokens.at(0), "after taking false branch here:"}
                                        .add(instruction.operands.at(2)->tokens.at(0)));
                            }
                        }
                    }
                }  // namespace checkers
            }                          // namespace static_analyser
        }                              // namespace frontend
    }                                  // namespace assembler
}  // namespace viua
