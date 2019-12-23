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
#include <viua/util/range.h>

using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_closure_instantiations(
    Register_usage_profile const& register_usage_profile,
    Parsed_source const& ps,
    std::map<Register, Closure> const& created_closures) -> void
{
    for (auto const& each : created_closures) {
        Register_usage_profile closure_register_usage_profile;
        auto const& fn = *std::find_if(ps.functions.begin(),
                                       ps.functions.end(),
                                       [&each](Instructions_block const& b) {
                                           return b.name == each.second.name;
                                       });
        for (auto& captured_value : each.second.defined_registers) {
            closure_register_usage_profile.define(captured_value.second.second,
                                                  captured_value.second.first);
        }

        // FIXME: This is ad-hoc code - move it to a utility function.
        auto const function_arity =
            std::stoul(fn.name.str().substr(fn.name.str().rfind('/') + 1));
        for (auto const i : viua::util::Range(
                 static_cast<viua::internals::types::register_index>(
                     function_arity))) {
            auto val         = Register{};
            val.index        = i;
            val.register_set = viua::internals::Register_sets::PARAMETERS;
            closure_register_usage_profile.define(val, fn.name);
        }

        try {
            map_names_to_register_indexes(closure_register_usage_profile, fn);
            check_register_usage_for_instruction_block_impl(
                closure_register_usage_profile,
                ps,
                fn,
                0,
                static_cast<InstructionIndex>(-1));
        } catch (Invalid_syntax& e) {
            throw Traced_syntax_error{}
                .append(e)
                .append(Invalid_syntax{fn.name, "in a closure defined here:"})
                .append(Invalid_syntax{
                    register_usage_profile.defined_where(each.first),
                    "when instantiated here:"});
        } catch (Traced_syntax_error& e) {
            throw e
                .append(Invalid_syntax{fn.name, "in a closure defined here:"})
                .append(Invalid_syntax{
                    register_usage_profile.defined_where(each.first),
                    "when instantiated here:"});
        }
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
