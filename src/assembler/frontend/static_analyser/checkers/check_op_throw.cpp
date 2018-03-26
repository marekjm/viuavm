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
auto check_op_throw(Register_usage_profile& register_usage_profile,
                    ParsedSource const& ps,
                    std::map<Register, Closure>& created_closures,
                    Instruction const& instruction) -> void {
    using viua::cg::lex::InvalidSyntax;
    using viua::cg::lex::TracedSyntaxError;

    auto source = get_operand<RegisterIndex>(instruction, 0);
    if (not source) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_use_of_register(register_usage_profile, *source, "throw from");
    assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
        register_usage_profile, *source);
    erase_if_direct_access(register_usage_profile, source, instruction);

    /*
     * If we reached a throw instruction there is no reason to continue
     * analysing instructions, as at runtime the execution of the function would
     * stop.
     */
    try {
        check_for_unused_registers(register_usage_profile);
        check_closure_instantiations(register_usage_profile, ps,
                                     created_closures);
    } catch (InvalidSyntax& e) {
        throw TracedSyntaxError{}.append(e).append(
            InvalidSyntax{instruction.tokens.at(0), "after a throw here:"});
    } catch (TracedSyntaxError& e) {
        throw e.append(
            InvalidSyntax{instruction.tokens.at(0), "after a throw here:"});
    }

    return;
}
}  // namespace checkers
}  // namespace static_analyser
}  // namespace frontend
}  // namespace assembler
}  // namespace viua
