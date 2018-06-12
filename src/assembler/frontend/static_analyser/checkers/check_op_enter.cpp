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
auto check_op_enter(Register_usage_profile& register_usage_profile,
                    Parsed_source const& ps,
                    Instruction const& instruction) -> void {
    using viua::assembler::frontend::parser::Label;
    using viua::cg::lex::Invalid_syntax;
    using viua::cg::lex::Traced_syntax_error;

    auto const label = get_operand<Label>(instruction, 0);
    if (not label) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected a block name");
    }

    auto const block_name = label->tokens.at(0).str();

    try {
        check_register_usage_for_instruction_block_impl(
            register_usage_profile, ps, ps.block(block_name), 0, 0);
    } catch (std::out_of_range const& e) {
        throw Invalid_syntax{label->tokens.at(0), "reference to undefined block: " + label->tokens.at(0).str()};
    } catch (Invalid_syntax& e) {
        throw TracedSyntaxError{}.append(e).append(Invalid_syntax{
            label->tokens.at(0), "after entering block " + block_name});
    } catch (Traced_syntax_error& e) {
        throw e.append(Invalid_syntax{label->tokens.at(0),
                                      "after entering block " + block_name});
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
