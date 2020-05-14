/*
 *  Copyright (C) 2020 Marek Marecki
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
auto check_op_exception(Register_usage_profile& register_usage_profile,
                           Instruction const& instruction) -> void
{
    auto target = get_operand<Register_index>(instruction, 0);
    if (not target) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    auto tag = get_operand<Register_index>(instruction, 1);
    if (not tag) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_use_of_register(register_usage_profile, *tag);
    assert_type_of_register<viua::internals::Value_types::ATOM>(
        register_usage_profile, *tag);

    auto value = get_operand<Register_index>(instruction, 2);
    if (not value) {
        using viua::assembler::frontend::parser::Void_literal;
        if (not get_operand<Void_literal>(instruction, 2)) {
            throw invalid_syntax(instruction.operands.at(2)->tokens,
                                 "invalid operand")
                .note("expected register index or void");
        }
    }

    if (value) {
        check_use_of_register(register_usage_profile, *value);

        if (value->as != viua::internals::Access_specifier::DIRECT) {
            throw invalid_syntax(instruction.operands.at(2)->tokens,
                                 "invalid access mode")
                .note("only direct access is legal when setting exception value");
        }
        erase_if_direct_access(register_usage_profile, value, instruction);
    }

    check_if_name_resolved(register_usage_profile, *target);

    auto val         = Register{};
    val.index        = target->index;
    val.register_set = target->rss;
    val.value_type   = viua::internals::Value_types::EXCEPTION;
    register_usage_profile.define(
        val, target->tokens.at(0), target->tokens.at(1));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
