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
auto check_op_tailcall(Register_usage_profile& register_usage_profile,
                       Instruction const& instruction) -> void {
    using viua::assembler::frontend::parser::Atom_literal;
    using viua::assembler::frontend::parser::Function_name_literal;
    using viua::assembler::frontend::parser::Void_literal;

    auto fn = instruction.operands.at(0).get();
    if ((not dynamic_cast<Atom_literal*>(fn))
        and (not dynamic_cast<Function_name_literal*>(fn))
        and (not dynamic_cast<Register_index*>(fn))) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected function name, atom literal, or register index");
    }
    if (auto r = dynamic_cast<Register_index*>(fn); r) {
        check_use_of_register(register_usage_profile, *r);
        assert_type_of_register<viua::internals::Value_types::INVOCABLE>(
            register_usage_profile, *r);
    }

    /*
     * Arguments are "consumed" by the callee, so from the static analyser's
     * point of view they are erased.
     */
    register_usage_profile.erase_arguments(instruction.tokens.at(0));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
