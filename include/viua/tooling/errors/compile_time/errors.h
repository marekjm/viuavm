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

#ifndef VIUA_TOOLING_ERRORS_COMPILE_TIME_ERRORS_H
#define VIUA_TOOLING_ERRORS_COMPILE_TIME_ERRORS_H

#include <memory>
#include <string>

#include <viua/tooling/errors/compile_time.h>
#include <viua/tooling/libs/lexer/tokenise.h>

namespace viua { namespace tooling { namespace errors { namespace compile_time {
// see struct Invalid_syntax
class Error {
    Compile_time_error const cause;

    viua::tooling::libs::lexer::Token const main_token;
    std::vector<viua::tooling::libs::lexer::Token> tokens;

    std::string const message;
    std::vector<std::string> attached_notes;
    std::vector<std::string> attached_comments;

    std::string aside_note;
    viua::tooling::libs::lexer::Token aside_token;

  public:
    auto line() const -> viua::tooling::libs::lexer::Token::Position_type;
    auto character() const -> viua::tooling::libs::lexer::Token::Position_type;

    auto note(std::string) -> Error&;
    auto notes() const -> std::vector<std::string> const&;

    auto comment(std::string) -> Error&;
    auto comments() const -> std::vector<std::string> const&;

    auto add(viua::tooling::libs::lexer::Token) -> Error&;
    auto match(viua::tooling::libs::lexer::Token const) const -> bool;

    auto aside(std::string) -> Error&;
    auto aside(viua::tooling::libs::lexer::Token, std::string) -> Error&;
    auto aside() const -> std::string;
    auto match_aside(viua::tooling::libs::lexer::Token) const -> bool;

    auto str() const -> std::string;
    auto what() const -> std::string;
    auto error_type() const -> Compile_time_error;

    auto empty() const -> bool;

    Error(Compile_time_error const,
          viua::tooling::libs::lexer::Token,
          std::string = "");
};

class Error_wrapper {
    std::vector<Error> fallout;

  public:
    auto append(Error) -> Error_wrapper&;
    auto errors() const -> std::vector<Error> const&;
    auto errors() -> std::vector<Error>&;
};
}}}}  // namespace viua::tooling::errors::compile_time

#endif
