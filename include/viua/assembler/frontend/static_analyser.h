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


#include <set>
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

                class Register_usage_profile {
                    /*
                     * Maps a register to the token that "defined" the register.
                     * Consider:
                     *
                     *      text %1 local "Hello World!"    ; register 1 is defined, and
                     *                                      ; the defining token is '%1'
                     */
                    std::map<Register, std::pair<viua::cg::lex::Token, Register>> defined_registers;

                    /*
                     * Registers are "fresh" until they either 1/ cross the boundary of an "if" instruction,
                     * or 2/ are used. After that they are no longer fresh. Overwriting a fresh register is an
                     * error because it means that the previously defined value is never used.
                     */
                    std::set<Register> fresh_registers;

                    /*
                     * Maps a register to the token marking the last place a register
                     * has been used.
                     * Consider:
                     *
                     *      text %1 local "Hello World!"    ; register 1 is defined, but unused
                     *      print %1 local                  ; register 1 becomes used
                     */
                    std::map<Register, viua::cg::lex::Token> used_registers;

                    /*
                     * Maps a register to the location where it was erased.
                     * Note that not every register must be erased, and
                     * when a register becomes defined it is no longer erased.
                     * Consider:
                     *
                     *      text %1 local "Hello World!"    ; register 1 is defined
                     *      delete %1 local                 ; register 1 is erased
                     *      integer %1 local 42              ; register 1 is defined again
                     */
                    std::map<Register, viua::cg::lex::Token> erased_registers;

                    /*
                     * The set of registers marked as "maybe unused".
                     * If a register is unused at the end of a block, but has an entry in
                     * this set an error should not be thrown for this register (but a warning, or
                     * a different type of diagnostic message can be emitted).
                     */
                    std::set<Register> maybe_unused_registers;

                    auto fresh(Register const) const -> bool;

                  public:
                    std::map<std::string, viua::internals::types::register_index> name_to_index;
                    std::map<viua::internals::types::register_index, std::string> index_to_name;

                    auto define(Register const r, viua::cg::lex::Token const t, bool const = false) -> void;
                    auto defined(Register const r) const -> bool;
                    auto defined_where(Register const r) const -> viua::cg::lex::Token;

                    auto infer(Register const r, viua::internals::ValueTypes const value_type_id,
                               viua::cg::lex::Token const& t) -> void;

                    auto at(Register const r) const -> const decltype(defined_registers)::mapped_type;

                    auto used(Register const r) const -> bool;
                    auto use(Register const r, viua::cg::lex::Token const t) -> void;

                    auto defresh() -> void;

                    auto erase(Register const r, viua::cg::lex::Token const& token) -> void;
                    auto erased(Register const r) const -> bool;
                    auto erased_where(Register const r) const -> viua::cg::lex::Token;

                    auto begin() const -> decltype(defined_registers.begin());
                    auto end() const -> decltype(defined_registers.end());
                };

                auto check_register_usage(const parser::ParsedSource&) -> void;
            }  // namespace static_analyser
        }      // namespace frontend
    }          // namespace assembler
}  // namespace viua

#endif
