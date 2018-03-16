/*
 *  Copyright (C) 2017 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_FRONTEND_STATIC_ANALYSER_H
#define VIUA_ASSEMBLER_FRONTEND_STATIC_ANALYSER_H


#include <string>
#include <viua/assembler/frontend/parser.h>


namespace viua {
    namespace assembler {
        namespace frontend {
            namespace static_analyser {
                auto verify_ress_instructions(const parser::ParsedSource&) -> void;
                auto verify_block_tries(const parser::ParsedSource&) -> void;
                auto verify_block_catches(const parser::ParsedSource&) -> void;
                auto verify_block_endings(const parser::ParsedSource& src) -> void;
                auto verify_frame_balance(const parser::ParsedSource& src) -> void;
                auto verify_function_call_arities(const parser::ParsedSource& src) -> void;
                auto verify_frames_have_no_gaps(const parser::ParsedSource& src) -> void;
                auto verify_jumps_are_in_range(const parser::ParsedSource&) -> void;

                auto verify(const parser::ParsedSource&) -> void;

                struct Register {
                    viua::internals::types::register_index index{0};
                    viua::internals::RegisterSets register_set = viua::internals::RegisterSets::LOCAL;
                    viua::internals::ValueTypes value_type = viua::internals::ValueTypes::UNDEFINED;
                    std::pair<bool, viua::cg::lex::Token> inferred = {false, {}};

                    auto operator<(const Register& that) const -> bool;
                    auto operator==(const Register& that) const -> bool;

                    Register() = default;
                    Register(viua::assembler::frontend::parser::RegisterIndex const&);
                };

                struct Closure {
                    std::string name;
                    std::map<Register, std::pair<viua::cg::lex::Token, Register>> defined_registers;

                    auto define(Register const, viua::cg::lex::Token const) -> void;

                    Closure();
                    Closure(std::string);
                };

                auto check_register_usage(const parser::ParsedSource&) -> void;
            }  // namespace static_analyser
        }      // namespace frontend
    }          // namespace assembler
}  // namespace viua

#endif
