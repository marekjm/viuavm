/*
 *  Copyright (C) 2025 Marek Marecki
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

#ifndef VIUA_SUPPORT_PRINT_HH
#define VIUA_SUPPORT_PRINT_HH

#include <stdio.h>

#include <filesystem>
#include <format>
#include <print>

#include <viua/support/tty.h>


namespace viua::support {
template<typename... Ts>
auto errorln(std::format_string<Ts...> fmt, Ts&&... args) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::send_escape_seq;

    std::println(
        stderr
        , "{}error{}: {}"
        , send_escape_seq(2, COLOR_FG_RED)
        , send_escape_seq(2, ATTR_RESET)
        , std::format(std::move(fmt), std::forward<Ts>(args)...));
}

template<typename... Ts>
auto errorln(std::filesystem::path const path, std::format_string<Ts...> fmt, Ts&&... args) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;

    std::println(
        stderr
        , "{}{}{}: {}error{}: {}"
        , send_escape_seq(2, COLOR_FG_WHITE)
        , path.native()
        , send_escape_seq(2, ATTR_RESET)
        , send_escape_seq(2, COLOR_FG_RED)
        , send_escape_seq(2, ATTR_RESET)
        , std::format(std::move(fmt), std::forward<Ts>(args)...));
}

template<typename... Ts>
auto noteln(std::filesystem::path const path, std::format_string<Ts...> fmt, Ts&&... args) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;

    std::println(
        stderr
        , "{}{}{}: {}note{}: {}"
        , send_escape_seq(2, COLOR_FG_WHITE)
        , path.native()
        , send_escape_seq(2, ATTR_RESET)
        , send_escape_seq(2, COLOR_FG_CYAN)
        , send_escape_seq(2, ATTR_RESET)
        , std::format(std::move(fmt), std::forward<Ts>(args)...));
}
}  // namespace viua::support

#endif

