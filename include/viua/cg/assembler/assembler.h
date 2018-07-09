/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_H
#define VIUA_ASSEMBLER_H

#pragma once


#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>
#include <viua/cg/lex.h>
#include <viua/program.h>

namespace assembler {
namespace operands {

auto resolve_register(viua::cg::lex::Token const,
                      bool const allow_bare_integers = false) -> std::string;
auto resolve_rs_type(viua::cg::lex::Token const)
    -> viua::internals::Register_sets;
auto resolve_jump(
    viua::cg::lex::Token const,
    std::map<std::string, std::vector<viua::cg::lex::Token>::size_type> const&,
    viua::internals::types::bytecode_size)
    -> std::tuple<viua::internals::types::bytecode_size, enum JUMPTYPE>;
auto convert_token_to_timeout_operand(viua::cg::lex::Token const) -> timeout_op;

auto getint(std::string const& s, bool const = false) -> int_op;
auto getint_with_rs_type(std::string const&,
                         const viua::internals::Register_sets,
                         bool const = false) -> int_op;
auto getint(std::vector<viua::cg::lex::Token> const& tokens,
            decltype(tokens.size())) -> int_op;
auto getbyte(std::string const& s) -> byte_op;
auto getfloat(std::string const& s) -> float_op;

auto normalise_binary_literal(std::string const s) -> std::string;
auto octal_to_binary_literal(std::string const s) -> std::string;
auto hexadecimal_to_binary_literal(std::string const s) -> std::string;
auto convert_token_to_bitstring_operand(const viua::cg::lex::Token)
    -> std::vector<uint8_t>;

auto get2(std::string s) -> std::tuple<std::string, std::string>;
auto get3(std::string s, bool fill_third = true)
    -> std::tuple<std::string, std::string, std::string>;
}  // namespace operands

namespace ce {
auto getmarks(std::vector<viua::cg::lex::Token> const& tokens)
    -> std::map<std::string,
                std::remove_reference<decltype(tokens)>::type::size_type>;
auto getlinks(std::vector<viua::cg::lex::Token> const&)
    -> std::vector<std::string>;

auto get_function_names(std::vector<viua::cg::lex::Token> const&)
    -> std::vector<std::string>;
auto get_signatures(std::vector<viua::cg::lex::Token> const&)
    -> std::vector<std::string>;
auto get_block_names(std::vector<viua::cg::lex::Token> const&)
    -> std::vector<std::string>;
auto get_block_signatures(std::vector<viua::cg::lex::Token> const&)
    -> std::vector<std::string>;
auto get_invokables(std::string const& type,
                    std::vector<viua::cg::lex::Token> const&)
    -> std::map<std::string, std::vector<std::string>>;
auto get_invokables_token_bodies(std::string const&,
                                 std::vector<viua::cg::lex::Token> const&)
    -> std::map<std::string, std::vector<viua::cg::lex::Token>>;
}  // namespace ce

namespace verify {
auto function_calls_are_defined(std::vector<viua::cg::lex::Token> const&,
                                std::vector<std::string> const&,
                                std::vector<std::string> const&) -> void;
auto callable_creations(std::vector<viua::cg::lex::Token> const&,
                        std::vector<std::string> const&,
                        std::vector<std::string> const&) -> void;
}  // namespace verify

namespace utils {
auto get_function_name_regex() -> std::regex;
auto is_valid_function_name(std::string const&) -> bool;
auto match_function_name(std::string const&) -> std::smatch;
auto get_function_arity(std::string const&) -> int;

namespace lines {
auto is_directive(std::string const&) -> bool;

auto is_function(std::string const&) -> bool;
auto is_closure(std::string const&) -> bool;
auto is_block(std::string const&) -> bool;
auto is_function_signature(std::string const&) -> bool;
auto is_block_signature(std::string const&) -> bool;
auto is_name(std::string const&) -> bool;
auto is_mark(std::string const&) -> bool;
auto is_info(std::string const&) -> bool;
auto is_end(std::string const&) -> bool;
auto is_import(std::string const&) -> bool;

auto make_function_signature(std::string const&) -> std::string;
auto make_block_signature(std::string const&) -> std::string;
auto make_info(std::string const&, std::string const&) -> std::string;
}  // namespace lines
}  // namespace utils
}  // namespace assembler


#endif
