/*
 *  Copyright (C) 2016, 2017, 2018 Marek Marecki
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

#ifndef VIUA_CG_LEX_H
#define VIUA_CG_LEX_H

#pragma once

#include <string>
#include <vector>


namespace viua { namespace cg { namespace lex {
class Token {
    std::string content, original_content;
    decltype(content.size()) line_number, character_in_line;

  public:
    auto line() const -> decltype(line_number);
    auto character() const -> decltype(character_in_line);

    auto str() const -> decltype(content);
    auto str(std::string) -> void;

    auto original() const -> decltype(original_content);
    auto original(std::string) -> void;

    auto ends(bool const = false) const -> decltype(character_in_line);

    auto operator==(std::string const& s) const -> bool;
    auto operator!=(std::string const& s) const -> bool;

    operator std::string() const;

    Token(decltype(line_number), decltype(character_in_line), std::string);
    Token();
};

struct Invalid_syntax {
    std::vector<Token>::size_type line_number, character_in_line;
    std::string content;
    std::string message;

    std::vector<Token> tokens;
    std::vector<std::string> attached_notes;
    std::string aside_note;
    Token aside_token;

    auto what() const -> const char*;
    auto str() const -> std::string;

    auto line() const -> decltype(line_number);
    auto character() const -> decltype(character_in_line);
    auto match(Token const) const -> bool;

    auto add(Token) -> Invalid_syntax&;

    auto note(std::string) -> Invalid_syntax&;
    auto notes() const -> const decltype(attached_notes)&;

    auto aside(std::string) -> Invalid_syntax&;
    auto aside(Token, std::string) -> Invalid_syntax&;
    auto aside() const -> std::string;
    auto match_aside(Token) const -> bool;

    Invalid_syntax(decltype(line_number),
                   decltype(character_in_line),
                   std::string);
    Invalid_syntax(Token, std::string = "");
};

struct Unused_value : public Invalid_syntax {
    Unused_value(Token);
    Unused_value(Token, std::string);
};

struct Traced_syntax_error {
    std::vector<Invalid_syntax> errors;

    auto what() const -> const char*;

    auto line() const -> decltype(errors.front().line());
    auto character() const -> decltype(errors.front().character());

    auto append(Invalid_syntax const&) -> Traced_syntax_error&;
};

auto is_reserved_keyword(std::string const&) -> bool;
auto is_mnemonic(std::string const&) -> bool;
auto assert_is_not_reserved_keyword(Token, std::string const&) -> void;

auto tokenise(std::string const&) -> std::vector<Token>;
auto standardise(std::vector<Token>) -> std::vector<Token>;
auto normalise(std::vector<Token>) -> std::vector<Token>;

template<class T, typename... R> bool adjacent(T first, T second) {
    if (first.line() != second.line()) {
        return false;
    }
    if (first.ends() != second.character()) {
        return false;
    }
    return true;
}
template<class T, typename... R> bool adjacent(T first, T second, R... rest) {
    if (first.line() != second.line()) {
        return false;
    }
    if (first.ends() != second.character()) {
        return false;
    }
    return adjacent(second, rest...);
}

auto join_tokens(std::vector<Token> const tokens,
                 decltype(tokens)::size_type const from,
                 decltype(from) const to) -> std::string;

auto reduce_token_sequence(std::vector<Token>, std::vector<std::string> const)
    -> std::vector<Token>;
auto reduce_directive(std::vector<Token>, std::string const)
    -> std::vector<Token>;
auto remove_spaces(std::vector<Token>) -> std::vector<Token>;
auto remove_comments(std::vector<Token>) -> std::vector<Token>;
auto reduce_newlines(std::vector<Token>) -> std::vector<Token>;
auto reduce_mark_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_name_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_info_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_import_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_function_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_closure_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_end_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_signature_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_bsignature_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_block_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_iota_directive(std::vector<Token>) -> std::vector<Token>;
auto reduce_double_colon(std::vector<Token>) -> std::vector<Token>;
auto reduce_left_attribute_bracket(std::vector<Token>) -> std::vector<Token>;
auto reduce_right_attribute_bracket(std::vector<Token>) -> std::vector<Token>;
auto reduce_function_signatures(std::vector<Token>) -> std::vector<Token>;
auto reduce_names(std::vector<Token>) -> std::vector<Token>;
auto reduce_offset_jumps(std::vector<Token>) -> std::vector<Token>;
auto reduce_at_prefixed_registers(std::vector<Token>) -> std::vector<Token>;
auto reduce_floats(std::vector<Token>) -> std::vector<Token>;

auto replace_iotas(std::vector<Token>) -> std::vector<Token>;
auto replace_defaults(std::vector<Token>) -> std::vector<Token>;
auto replace_named_registers(std::vector<Token>) -> std::vector<Token>;
auto move_inline_blocks_out(std::vector<Token>) -> std::vector<Token>;
auto unwrap_lines(std::vector<Token>, bool full = true) -> std::vector<Token>;

auto cook(std::vector<Token>, bool const = true) -> std::vector<Token>;
}}}  // namespace viua::cg::lex


#endif
