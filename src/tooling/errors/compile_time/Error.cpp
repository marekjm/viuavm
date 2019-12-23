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

#include <viua/tooling/errors/compile_time.h>
#include <viua/tooling/errors/compile_time/errors.h>

namespace viua { namespace tooling { namespace errors { namespace compile_time {
Error::Error(Compile_time_error const e,
             viua::tooling::libs::lexer::Token t,
             std::string m)
        : cause{e}, main_token{t}, message{m}
{}

auto Error::line() const -> viua::tooling::libs::lexer::Token::Position_type
{
    return main_token.line();
}
auto Error::character() const
    -> viua::tooling::libs::lexer::Token::Position_type
{
    return main_token.character();
}

auto Error::note(std::string n) -> Error&
{
    attached_notes.emplace_back(std::move(n));
    return *this;
}

auto Error::notes() const -> std::vector<std::string> const&
{
    return attached_notes;
}

auto Error::comment(std::string n) -> Error&
{
    attached_comments.emplace_back(std::move(n));
    return *this;
}

auto Error::comments() const -> std::vector<std::string> const&
{
    return attached_comments;
}

auto Error::add(viua::tooling::libs::lexer::Token const token) -> Error&
{
    tokens.push_back(token);
    return *this;
}
auto Error::match(viua::tooling::libs::lexer::Token const token) const -> bool
{
    if (token.line() == main_token.line()
        and token.character() >= main_token.character()
        and token.ends(true) <= main_token.ends(true)) {
        return true;
    }
    for (auto const& each : tokens) {
        if (token.line() != each.line()) {
            continue;
        }
        if (token.character() >= each.character()
            and token.ends(true) <= each.ends(true)) {
            return true;
        }
    }
    return false;
}

auto Error::aside(std::string a) -> Error&
{
    aside_note  = a;
    aside_token = main_token;
    return *this;
}
auto Error::aside(viua::tooling::libs::lexer::Token t, std::string a) -> Error&
{
    aside_note  = a;
    aside_token = t;
    return *this;
}
auto Error::aside() const -> std::string { return aside_note; }
auto Error::match_aside(viua::tooling::libs::lexer::Token token) const -> bool
{
    if (token.line() == aside_token.line()
        and token.character() == aside_token.character()) {
        return true;
    }
    if (token.line() != aside_token.line()) {
        return false;
    }
    if (token.character() >= aside_token.character()
        and token.ends(true) <= aside_token.ends(true)) {
        return true;
    }
    return false;
}

auto Error::str() const -> std::string { return message; }
auto Error::what() const -> std::string
{
    if (auto s = viua::tooling::errors::compile_time::display_error(cause);
        s.empty()) {
        return message;
    } else {
        return s + (message.empty() ? "" : (": " + message));
    }
}

auto Error::empty() const -> bool
{
    return (cause == Compile_time_error::Empty_error) and message.empty();
}
}}}}  // namespace viua::tooling::errors::compile_time
