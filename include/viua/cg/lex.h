/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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


namespace viua {
    namespace cg {
        namespace lex {
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

                auto ends() const -> decltype(character_in_line);

                bool operator==(const std::string& s) const;
                bool operator!=(const std::string& s) const;

                operator std::string() const;

                Token(decltype(line_number), decltype(character_in_line), std::string);
                Token();
            };

            struct InvalidSyntax {
                std::vector<Token>::size_type line_number, character_in_line;
                std::string content;
                std::string message;

                std::vector<Token> tokens;

                const char* what() const;

                auto line() const -> decltype(line_number);
                auto character() const -> decltype(character_in_line);
                auto match(Token) const -> bool;

                auto add(Token) -> InvalidSyntax&;

                InvalidSyntax(decltype(line_number), decltype(character_in_line), std::string);
                InvalidSyntax(Token, std::string = "");
            };

            struct UnusedValue: public InvalidSyntax {
                UnusedValue(Token);
            };

            struct TracedSyntaxError {
                std::vector<InvalidSyntax> errors;

                const char* what() const;

                auto line() const -> decltype(errors.front().line());
                auto character() const -> decltype(errors.front().character());

                auto append(const InvalidSyntax&) -> TracedSyntaxError&;
            };

            bool is_reserved_keyword(const std::string&);
            void assert_is_not_reserved_keyword(Token, const std::string&);

            std::vector<Token> tokenise(const std::string&);
            std::vector<Token> standardise(std::vector<Token>);

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

            std::string join_tokens(const std::vector<Token> tokens, const decltype(tokens)::size_type from, const decltype(from) to);

            std::vector<Token> remove_spaces(std::vector<Token>);
            std::vector<Token> remove_comments(std::vector<Token>);
            std::vector<Token> reduce_newlines(std::vector<Token>);
            std::vector<Token> reduce_mark_directive(std::vector<Token>);
            std::vector<Token> reduce_name_directive(std::vector<Token>);
            std::vector<Token> reduce_info_directive(std::vector<Token>);
            std::vector<Token> reduce_main_directive(std::vector<Token>);
            std::vector<Token> reduce_link_directive(std::vector<Token>);
            std::vector<Token> reduce_function_directive(std::vector<Token>);
            std::vector<Token> reduce_closure_directive(std::vector<Token>);
            std::vector<Token> reduce_end_directive(std::vector<Token>);
            std::vector<Token> reduce_signature_directive(std::vector<Token>);
            std::vector<Token> reduce_bsignature_directive(std::vector<Token>);
            std::vector<Token> reduce_block_directive(std::vector<Token>);
            std::vector<Token> reduce_iota_directive(std::vector<Token>);
            std::vector<Token> reduce_double_colon(std::vector<Token>);
            std::vector<Token> reduce_function_signatures(std::vector<Token>);
            std::vector<Token> reduce_names(std::vector<Token>);
            std::vector<Token> reduce_offset_jumps(std::vector<Token>);
            std::vector<Token> reduce_at_prefixed_registers(std::vector<Token>);
            std::vector<Token> reduce_floats(std::vector<Token>);

            std::vector<Token> replace_iotas(std::vector<Token>);
            std::vector<Token> replace_defaults(std::vector<Token>);
            std::vector<Token> replace_named_registers(std::vector<Token>);
            std::vector<Token> move_inline_blocks_out(std::vector<Token>);
            std::vector<Token> unwrap_lines(std::vector<Token>, bool full = true);

            std::vector<Token> cook(std::vector<Token>, const bool = true);
        }
    }
}


#endif
