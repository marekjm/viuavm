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

#include <string>
#include <unistd.h>
#include <viua/util/string/escape_sequences.h>

namespace viua {
    namespace util {
        namespace string {
            namespace escape_sequences {
auto send_escape_seq(std::string const& seq) -> std::string
{
    auto is_terminal = isatty(1);
    std::string env_color_flag{
        getenv("VIUAVM_ASM_COLOUR") ? getenv("VIUAVM_ASM_COLOUR") : "default"};

    bool colorise = is_terminal;
    if (env_color_flag == "default") {
        // do nothing; the default is to colorise when printing to teminal and
        // do not colorise otherwise
    }
    else if (env_color_flag == "never") {
        colorise = false;
    }
    else if (env_color_flag == "always") {
        colorise = true;
    }
    else {
        // unknown value, do nothing
    }

    if (colorise) {
        return seq;
    }
    return "";
}
}}}}  // namespace viua::util::string::escape_sequences
