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
auto check_op_bitset(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void {
    using viua::assembler::frontend::parser::Bits_literal;
    using viua::assembler::frontend::parser::Boolean_literal;

    auto target = get_operand<Register_index>(instruction, 0);
    if (not target) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_use_of_register(register_usage_profile, *target);
    assert_type_of_register<viua::internals::Value_types::BITS>(
        register_usage_profile, *target);

    auto index = get_operand<Register_index>(instruction, 1);
    if (not index) {
        if (not get_operand<Bits_literal>(instruction, 1)) {
            throw invalid_syntax(instruction.operands.at(1)->tokens,
                                 "invalid operand")
                .note("expected register index or bits literal");
        }
    }

    check_use_of_register(register_usage_profile, *index);
    assert_type_of_register<viua::internals::Value_types::INTEGER>(
        register_usage_profile, *index);

    auto value = get_operand<Register_index>(instruction, 2);
    if (not value) {
        if (not get_operand<Boolean_literal>(instruction, 2)) {
            throw invalid_syntax(instruction.operands.at(0)->tokens,
                                 "invalid operand")
                .note("expected register index or boolean literal");
        }
    }

    if (value) {
        check_use_of_register(register_usage_profile, *value);
        assert_type_of_register<viua::internals::Value_types::BOOLEAN>(
            register_usage_profile, *value);
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
