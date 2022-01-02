/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#include <stdlib.h>
#include <unistd.h>

#include <string_view>

#include <viua/support/tty.h>


namespace viua::support::tty {
auto send_escape_seq(int const fd, std::string_view const seq)
    -> std::string_view
{
    auto const colour_flag = std::string_view{
        getenv("VIUA_COLOUR") ? getenv("VIUA_COLOUR") : "default"};

    auto apply_colour = static_cast<bool>(isatty(fd));
    if (colour_flag == "default") {
        /*
         * By default, if output stream is a TTY apply colours and other
         * escape sequences.
         */
    } else if (colour_flag == "never") {
        apply_colour = false;
    } else if (colour_flag == "always") {
        apply_colour = true;
    }

    if (apply_colour) {
        return seq;
    }

    return "";
}
}  // namespace viua::support::tty
