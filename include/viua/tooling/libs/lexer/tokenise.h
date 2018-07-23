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

#ifndef VIUA_TOOLING_LIBS_LEXER_TOKENISE_H
#define VIUA_TOOLING_LIBS_LEXER_TOKENISE_H

#include <string>
#include <vector>

namespace viua {
namespace tooling {
namespace libs {
namespace lexer {
class Token {
    std::string content, original_content;

  public:
    using Position_type = decltype(content)::size_type;

  private:
    Position_type const line_number;
    Position_type const character_in_line;

  public:
    auto line() const -> Position_type;
    auto character() const -> Position_type;

    auto str() const -> decltype(content);
    auto str(std::string) -> void;

    auto original() const -> decltype(original_content);
    auto original(std::string) -> void;

    auto ends(bool const = false) const -> Position_type;

    auto operator==(std::string const& s) const -> bool;
    auto operator!=(std::string const& s) const -> bool;

    explicit operator std::string() const;

    Token(Position_type const, Position_type const, std::string);
    Token();
};

auto tokenise(std::string const&) -> std::vector<Token>;
auto strip_spaces(std::vector<Token> const&) -> std::vector<Token>;
}}}}

#endif
