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
#include <sstream>
#include <string>
#include <vector>
#include <viua/util/string/ops.h>
#include <viua/tooling/libs/lexer/tokenise.h>

namespace viua {
namespace tooling {
namespace libs {
namespace lexer {
auto Token::line() const -> Position_type {
    return line_number;
}
auto Token::character() const -> Position_type {
    return character_in_line;
}

auto Token::str() const -> decltype(content) {
    return content;
}
auto Token::str(std::string s) -> void {
    content = s;
}

auto Token::original() const -> decltype(original_content) {
    return original_content;
}
auto Token::original(std::string s) -> void {
    original_content = s;
}

auto Token::ends(bool const as_original) const -> Position_type {
    return (character_in_line
            + (as_original ? original_content : content).size());
}

auto Token::operator==(std::string const& s) const -> bool {
    return (content == s);
}
auto Token::operator!=(std::string const& s) const -> bool {
    return (content != s);
}

Token::operator std::string() const {
    return str();
}

Token::Token(Position_type const line,
             Position_type const character,
             std::string text)
        : content{ text }
        , original_content{ text }
        , line_number{ line }
        , character_in_line{ character } {}
Token::Token() : Token(0, 0, "") {}

auto tokenise(std::string const& source) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    std::ostringstream candidate_token;
    candidate_token.str("");

    Token::Position_type line_number       = 0;
    Token::Position_type character_in_line = 0;
    auto const limit                       = source.size();

    uint64_t hyphens    = 0;
    bool active_comment = false;
    for (decltype(source.size()) i = 0; i < limit; ++i) {
        auto const current_char       = source.at(i);
        auto found_breaking_character = false;

        switch (current_char) {
        case '\n':
        case '-':
        case '\t':
        case ' ':
        case '~':
        case '`':
        case '!':
        case '@':
        case '#':
        case '$':
        case '%':
        case '^':
        case '&':
        case '*':
        case '(':
        case ')':
        case '+':
        case '=':
        case '{':
        case '}':
        case '[':
        case ']':
        case '|':
        case ':':
        case ';':
        case ',':
        case '.':
        case '<':
        case '>':
        case '/':
        case '?':
        case '\'':
        case '"':
            found_breaking_character = true;
            break;
        default:
            candidate_token << source.at(i);
        }

        if (current_char == ';') {
            active_comment = true;
        }
        if (current_char == '-') {
            if (++hyphens >= 2) {
                active_comment = true;
            }
        } else {
            hyphens = 0;
        }

        if (found_breaking_character) {
            if (candidate_token.str().size()) {
                tokens.emplace_back(
                    line_number, character_in_line, candidate_token.str());
                character_in_line += candidate_token.str().size();
                candidate_token.str("");
            }
            if ((current_char == '\'' or current_char == '"')
                and not active_comment) {
                auto const s = viua::util::string::ops::extract(source.substr(i));

                tokens.emplace_back(line_number, character_in_line, s);
                character_in_line += (s.size() - 1);
                i += (s.size() - 1);
            } else {
                candidate_token << current_char;
                tokens.emplace_back(
                    line_number, character_in_line, candidate_token.str());
                candidate_token.str("");
            }

            ++character_in_line;
            if (current_char == '\n') {
                ++line_number;
                character_in_line = 0;
                active_comment    = false;
            }
        }
    }

    return tokens;
}
auto strip_spaces(std::vector<Token> const& source) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    auto const space = std::string{" "};
    auto const tab = std::string{"\t"};
    std::copy_if(source.begin(), source.end(), std::back_inserter(tokens),
    [&space, &tab](Token const& each) -> bool {
        auto const s = each.str();
        return (s != space) and (s != tab);
    });

    return tokens;
}
}}}}
