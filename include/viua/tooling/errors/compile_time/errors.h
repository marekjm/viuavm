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
#include <viua/tooling/libs/lexer/tokenise.h>
#include <viua/tooling/errors/compile_time.h>

namespace viua {
namespace tooling {
namespace errors {
namespace compile_time {
// see struct Invalid_syntax
class Error {
    Compile_time_error const cause;

    viua::tooling::libs::lexer::Token const main_token;
    std::vector<viua::tooling::libs::lexer::Token> tokens;

    std::string const message;
    std::vector<std::string> attached_notes;

    std::string aside_note;
    viua::tooling::libs::lexer::Token const aside_token;

  public:
    auto what() const -> std::string;
    auto error_type() const -> Compile_time_error;

    Error(Compile_time_error const, viua::tooling::libs::lexer::Token, std::string = "");
};

class Error_wrapper {
        std::vector<Error> errors;

    public:
        auto append(Error) -> Error_wrapper&;
};
}}}}

#endif
