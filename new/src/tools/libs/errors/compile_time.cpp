/*
 *  Copyright (C) 2018, 2021-2022 Marek Marecki
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

#include <viua/libs/errors/compile_time.h>


namespace viua::libs::errors::compile_time {
auto to_string(Cause const ce) -> std::string_view
{
    switch (ce) {
    case Cause::Unknown:
        return "unknown error";
    case Cause::None:
        return "";
    case Cause::Illegal_character:
        return "illegal character";
    case Cause::Invalid_token:
        return "invalid token";
    case Cause::Unexpected_token:
        return "unexpected token";
    case Cause::Unknown_directive:
        return "unknown directive";
    case Cause::Unknown_opcode:
        return "unknown opcode";
    case Cause::Invalid_register_access:
        return "invalid register access";
    case Cause::Too_few_operands:
        return "too few operands";
    case Cause::Call_to_undefined_function:
        return "call to undefined function";
    case Cause::Value_out_of_range:
        return "value out of range";
    case Cause::Invalid_operand:
        return "invalid operand";
    case Cause::Duplicated_entry_point:
        return "duplicated entry point";
    case Cause::Unknown_type:
        return "unknown type";
    case Cause::Unknown_label:
        return "unknown label";
    }
    return "illegal error";
}

Error::Error(viua::libs::lexer::Lexeme lx, Cause const ce, std::string m)
        : cause{ce}, message{std::move(m)}, main_lexeme{lx}
{}

auto Error::chain(Error&& e) -> Error&
{
    fallout.emplace_back(std::move(e));
    return *this;
}

auto Error::aside(std::string s, std::optional<Lexeme> l) -> Error&
{
    aside_note   = std::move(s);
    aside_lexeme = l;
    if (l.has_value()) {
        add(*l);
    }
    return *this;
}
auto Error::aside() const -> std::string_view
{
    return aside_note;
}

auto Error::note(std::string s) & -> Error&
{
    attached_notes.emplace_back(std::move(s));
    return *this;
}
auto Error::note(std::string s) && -> Error
{
    attached_notes.emplace_back(std::move(s));
    return std::move(*this);
}
auto Error::notes() const -> std::vector<std::string> const&
{
    return attached_notes;
}

auto Error::add(Lexeme lx, bool const advisory) & -> Error&
{
    detail_lexemes.emplace_back(lx, advisory);
    return *this;
}
auto Error::add(Lexeme lx, bool const advisory) && -> Error
{
    detail_lexemes.emplace_back(lx, advisory);
    return std::move(*this);
}

auto Error::spans() const -> std::vector<span_type>
{
    auto ss = std::vector<span_type>{};
    ss.emplace_back(
        main_lexeme.location.character, main_lexeme.text.size(), false);
    for (auto const& each : detail_lexemes) {
        ss.emplace_back(
            each.first.location.character, each.first.text.size(), each.second);
    }
    std::sort(ss.begin(), ss.end());
    return ss;
}

auto Error::what() const -> std::string_view
{
    return to_string(cause);
}

auto Error::str() const -> std::string
{
    auto out = std::ostringstream{};
    if (cause != Cause::None) {
        out << to_string(cause);
        if (not message.empty()) {
            out << ": ";
        }
    }
    out << message;
    return out.str();
}
}  // namespace viua::libs::errors::compile_time
