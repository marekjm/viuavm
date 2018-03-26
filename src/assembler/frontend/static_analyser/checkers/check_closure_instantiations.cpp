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

#include <algorithm>
#include <viua/assembler/frontend/static_analyser.h>

using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::Token;
using viua::cg::lex::TracedSyntaxError;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_closure_instantiations(
    Register_usage_profile const& register_usage_profile,
    ParsedSource const& ps,
    std::map<Register, Closure> const& created_closures) -> void {
    for (const auto& each : created_closures) {
        Register_usage_profile closure_register_usage_profile;
        const auto& fn = *std::find_if(ps.functions.begin(),
                                       ps.functions.end(),
                                       [&each](const InstructionsBlock& b) {
                                           return b.name == each.second.name;
                                       });
        for (auto& captured_value : each.second.defined_registers) {
            closure_register_usage_profile.define(captured_value.second.second,
                                                  captured_value.second.first);
        }

        try {
            map_names_to_register_indexes(closure_register_usage_profile, fn);
            check_register_usage_for_instruction_block_impl(
                closure_register_usage_profile,
                ps,
                fn,
                0,
                static_cast<InstructionIndex>(-1));
        } catch (InvalidSyntax& e) {
            throw TracedSyntaxError{}
                .append(e)
                .append(InvalidSyntax{fn.name, "in a closure defined here:"})
                .append(InvalidSyntax{
                    register_usage_profile.defined_where(each.first),
                    "when instantiated here:"});
        } catch (TracedSyntaxError& e) {
            throw e.append(InvalidSyntax{fn.name, "in a closure defined here:"})
                .append(InvalidSyntax{
                    register_usage_profile.defined_where(each.first),
                    "when instantiated here:"});
        }
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
