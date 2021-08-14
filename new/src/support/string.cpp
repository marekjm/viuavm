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

#include <viua/support/string.h>

#include <iomanip>
#include <string>
#include <sstream>
#include <string_view>


namespace viua::support::string {
    auto quoted(std::string_view const sv) -> std::string
    {
        auto out = std::ostringstream{};
        out << std::quoted(sv);

        auto const tmp = out.str();
        out.str("");

        for (auto const each : tmp) {
            if (each == '\n') {
                out << "\\n";
            } else {
                out << each;
            }
        }

        return out.str();
    }

    auto quote_squares(std::string_view const sv) -> std::string
    {
        auto out = std::ostringstream{};
        out << CORNER_QUOTE_LL << sv << CORNER_QUOTE_UR;
        return out.str();
    }

    auto unescape(std::string_view const sv) -> std::string
    {
        /*
         * Decode escape sequences in strings.
         *
         * This function recognizes escape sequences as listed on:
         * http://en.cppreference.com/w/cpp/language/escape
         * The function does not recognize sequences for:
         *     - arbitrary octal numbers (escape: \nnn),
         *     - arbitrary hexadecimal numbers (escape: \xnn),
         *     - short arbitrary Unicode values (escape: \unnnn),
         *     - long arbitrary Unicode values (escape: \Unnnnnnnn),
         *
         * If a character that does not encode an escape sequence is
         * preceded by a backslash (\\) the function consumes the backslash and
         * leaves only the "escaped" character in the output std::string.
         */
        auto decoded = std::ostringstream{};
        auto c       = char{};
        for (auto i = std::string::size_type{0}; i < sv.size(); ++i) {
            c = sv[i];
            if (c == '\\' and i < (sv.size() - 1)) {
                ++i;
                switch (sv[i]) {
                case '\'':
                    c = '\'';
                    break;
                case '"':
                    c = '"';
                    break;
                case '?':
                    c = '?';
                    break;
                case '\\':
                    c = '\\';
                    break;
                case 'a':
                    c = '\a';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'v':
                    c = '\v';
                    break;
                default:
                    c = sv[i];
                }
            }
            decoded << c;
        }
        return decoded.str();
    }
}
