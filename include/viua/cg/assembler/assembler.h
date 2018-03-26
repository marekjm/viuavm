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
auto getint(const std::string& s, const bool = false) -> int_op;
auto getint_with_rs_type(const std::string&,
                         const viua::internals::RegisterSets,
                         const bool = false) -> int_op;
auto getint(const std::vector<viua::cg::lex::Token>& tokens,
            decltype(tokens.size())) -> int_op;
auto getbyte(const std::string& s) -> byte_op;
auto getfloat(const std::string& s) -> float_op;

auto normalise_binary_literal(const std::string s) -> std::string;
auto octal_to_binary_literal(const std::string s) -> std::string;
auto hexadecimal_to_binary_literal(const std::string s) -> std::string;
auto convert_token_to_bitstring_operand(const viua::cg::lex::Token)
    -> std::vector<uint8_t>;

auto get2(std::string s) -> std::tuple<std::string, std::string>;
auto get3(std::string s, bool fill_third = true)
    -> std::tuple<std::string, std::string, std::string>;
}  // namespace operands

namespace ce {
auto getmarks(const std::vector<viua::cg::lex::Token>& tokens)
    -> std::map<std::string,
                std::remove_reference<decltype(tokens)>::type::size_type>;
auto getlinks(const std::vector<viua::cg::lex::Token>&)
    -> std::vector<std::string>;

auto get_function_names(const std::vector<viua::cg::lex::Token>&)
    -> std::vector<std::string>;
auto get_signatures(const std::vector<viua::cg::lex::Token>&)
    -> std::vector<std::string>;
auto get_block_names(const std::vector<viua::cg::lex::Token>&)
    -> std::vector<std::string>;
auto get_block_signatures(const std::vector<viua::cg::lex::Token>&)
    -> std::vector<std::string>;
auto get_invokables(const std::string& type,
                    const std::vector<viua::cg::lex::Token>&)
    -> std::map<std::string, std::vector<std::string>>;
auto get_invokables_token_bodies(const std::string&,
                                 const std::vector<viua::cg::lex::Token>&)
    -> std::map<std::string, std::vector<viua::cg::lex::Token>>;
}  // namespace ce

namespace verify {
auto function_calls_are_defined(const std::vector<viua::cg::lex::Token>&,
                                const std::vector<std::string>&,
                                const std::vector<std::string>&) -> void;
auto callable_creations(const std::vector<viua::cg::lex::Token>&,
                        const std::vector<std::string>&,
                        const std::vector<std::string>&) -> void;
auto manipulation_of_defined_registers(
    const std::vector<viua::cg::lex::Token>&,
    const std::map<std::string, std::vector<viua::cg::lex::Token>>&,
    const bool) -> void;
}  // namespace verify

namespace utils {
auto get_function_name_regex() -> std::regex;
auto is_valid_function_name(const std::string&) -> bool;
auto match_function_name(const std::string&) -> std::smatch;
auto get_function_arity(const std::string&) -> int;

namespace lines {
auto is_directive(const std::string&) -> bool;

auto is_function(const std::string&) -> bool;
auto is_closure(const std::string&) -> bool;
auto is_block(const std::string&) -> bool;
auto is_function_signature(const std::string&) -> bool;
auto is_block_signature(const std::string&) -> bool;
auto is_name(const std::string&) -> bool;
auto is_mark(const std::string&) -> bool;
auto is_info(const std::string&) -> bool;
auto is_end(const std::string&) -> bool;
auto is_import(const std::string&) -> bool;

auto make_function_signature(const std::string&) -> std::string;
auto make_block_signature(const std::string&) -> std::string;
auto make_info(const std::string&, const std::string&) -> std::string;
}  // namespace lines
}  // namespace utils
}  // namespace assembler


#endif
