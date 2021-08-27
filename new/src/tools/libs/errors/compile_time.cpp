/*
 *  Copyright (C) 2018, 2021 Marek Marecki
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
    }
    return "illegal error";
}

Error::Error(viua::libs::lexer::Lexeme lx, Cause const ce, std::string m)
        : cause{ce}, message{std::move(m)}, main_lexeme{lx}
{}

auto Error::aside(std::string s) -> Error&
{
    aside_note = std::move(s);
    return *this;
}
auto Error::aside() const -> std::string_view
{
    return aside_note;
}

auto Error::note(std::string s) -> Error&
{
    attached_notes.emplace_back(std::move(s));
    return *this;
}
auto Error::notes() const -> std::vector<std::string> const&
{
    return attached_notes;
}

auto Error::add(Lexeme lx) -> Error&
{
    detail_lexemes.push_back(lx);
    return *this;
}
auto Error::spans() const -> std::vector<span_type>
{
    auto ss = std::vector<span_type>{};
    ss.emplace_back(main_lexeme.location.character, main_lexeme.text.size());
    for (auto const& each : detail_lexemes) {
        ss.emplace_back(each.location.character, each.text.size());
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
