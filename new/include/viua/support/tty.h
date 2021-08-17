/*
 *  Copyright (C) 2021 Marek Marecki
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

#include <string_view>

namespace viua::support::tty {
constexpr auto COLOR_FG_RED          = std::string_view{"\x1b[38;5;1m"};
constexpr auto COLOR_FG_YELLOW       = std::string_view{"\x1b[38;5;3m"};
constexpr auto COLOR_FG_CYAN         = std::string_view{"\x1b[38;5;6m"};
constexpr auto COLOR_FG_LIGHT_GREEN  = std::string_view{"\x1b[38;5;10m"};
constexpr auto COLOR_FG_LIGHT_YELLOW = std::string_view{"\x1b[38;5;11m"};
constexpr auto COLOR_FG_WHITE        = std::string_view{"\x1b[38;5;15m"};
constexpr auto COLOR_FG_GREEN_1      = std::string_view{"\x1b[38;5;46m"};
constexpr auto COLOR_FG_RED_1        = std::string_view{"\x1b[38;5;196m"};
constexpr auto COLOR_FG_ORANGE_RED_1 = std::string_view{"\x1b[38;5;202m"};
constexpr auto ATTR_RESET            = std::string_view{"\x1b[0m"};

auto send_escape_seq(int const, std::string_view const) -> std::string_view;
}  // namespace viua::support::tty
