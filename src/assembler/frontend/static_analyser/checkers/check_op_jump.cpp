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
                    auto check_op_jump(Register_usage_profile& register_usage_profile, ParsedSource const& ps,
                                       Instruction const& instruction, InstructionsBlock const& ib,
                                       InstructionIndex i, InstructionIndex const mnemonic_counter) -> void {
                        using viua::assembler::frontend::parser::Label;
                        using viua::assembler::frontend::parser::Offset;
                        using viua::cg::lex::InvalidSyntax;

                        auto target = instruction.operands.at(0).get();

                        if (auto offset = dynamic_cast<Offset*>(target); offset) {
                            auto const jump_target = std::stol(offset->tokens.at(0));
                            if (jump_target > 0) {
                                i = get_line_index_of_instruction(
                                    mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                                check_register_usage_for_instruction_block_impl(register_usage_profile, ps,
                                                                                ib, i, mnemonic_counter);
                            }
                        } else if (auto label = dynamic_cast<Label*>(target); label) {
                            auto jump_target = ib.marker_map.at(label->tokens.at(0));
                            jump_target = get_line_index_of_instruction(jump_target, ib);
                            if (jump_target > i) {
                                check_register_usage_for_instruction_block_impl(
                                    register_usage_profile, ps, ib, jump_target, mnemonic_counter);
                            }
                        } else if (str::ishex(target->tokens.at(0))) {
                            // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump
                            // instructions. Do not check them now, but this should be fixed in the future.
                        } else {
                            throw InvalidSyntax(target->tokens.at(0), "invalid operand for jump instruction");
                        }
                    }
                }  // namespace checkers
            }      // namespace static_analyser
        }          // namespace frontend
    }              // namespace assembler
}  // namespace viua
