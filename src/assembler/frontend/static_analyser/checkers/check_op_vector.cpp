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
auto check_op_vector(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void {
    using viua::cg::lex::Invalid_syntax;

    auto operand = get_operand<Register_index>(instruction, 0);
    if (not operand) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    auto const pack_range_start = get_operand<Register_index>(instruction, 1);
    if (not pack_range_start) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    auto const pack_range_count = get_operand<Register_index>(instruction, 2);
    if (not pack_range_count) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_if_name_resolved(register_usage_profile, *operand);

    auto const limit = (pack_range_start->index + pack_range_count->index);
    for (auto j = pack_range_start->index; j < limit; ++j) {
        auto checker         = Register{};
        checker.index        = j;
        checker.register_set = pack_range_start->rss;
        if (not register_usage_profile.defined(checker)) {
            throw Invalid_syntax{pack_range_start->tokens.at(0),
                                 "pack of empty "
                                     + to_string(pack_range_start->rss)
                                     + " register "
                                     + std::to_string(j)};
        }
        register_usage_profile.erase(checker, instruction.tokens.at(0));
        register_usage_profile.use(checker, instruction.tokens.at(0));
    }

    auto val         = Register{};
    val.index        = operand->index;
    val.register_set = operand->rss;
    val.value_type   = viua::internals::Value_types::VECTOR;
    register_usage_profile.define(val, operand->tokens.at(0));
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
