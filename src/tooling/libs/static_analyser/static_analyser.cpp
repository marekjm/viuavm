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

#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/static_analyser/static_analyser.h>

namespace viua {
namespace tooling {
namespace libs {
namespace static_analyser {

static auto analyse_single_function(
    viua::tooling::libs::parser::Cooked_function const& fn
    , viua::tooling::libs::parser::Cooked_fragments const&
    , Analyser_state&
) -> void {
    using viua::tooling::libs::parser::Fragment_type;
    using viua::tooling::libs::parser::Instruction;
    if (auto const& l = *fn.body().at(0); not (l.type() == Fragment_type::Instruction
                and static_cast<Instruction const&>(l).opcode == ALLOCATE_REGISTERS)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Must_allocate_registers_first
                , l.token(0)
            }.note("the first instruction of every function must be `allocate_registers'"));
    }
}

static auto analyse_functions(
    viua::tooling::libs::parser::Cooked_fragments const& fragments
    , Analyser_state& analyser_state
) -> void {
    auto const& functions = fragments.function_fragments;

    for (auto const& [ name, fn ] : functions) {
        try {
            analyse_single_function(fn, fragments, analyser_state);
        } catch (viua::tooling::errors::compile_time::Error_wrapper& e) {
            e.append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , fn.head().token(0)
                , "in function `" + name + "'"
            });
            throw;
        }
    }
}

auto analyse(viua::tooling::libs::parser::Cooked_fragments const& fragments) -> void {
    auto analyser_state = Analyser_state{};

    analyse_functions(fragments, analyser_state);

    /* analyse_closure_instantiations(fragments); */
}

}}}}
