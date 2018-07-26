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
#include <regex>
#include <viua/util/string/ops.h>
#include <viua/tooling/libs/lexer/classifier.h>

namespace viua {
namespace tooling {
namespace libs {
namespace lexer {
namespace classifier {
auto is_id(std::string const& s) -> bool {
    /*  Returns true if s is a valid identifier.
     */
    static auto const identifier = std::regex{"^[a-zA-Z_][a-zA-Z0-9_]*$"};
    return regex_match(s, identifier);
}
auto is_decimal_integer(std::string const& s) -> bool {
    /*  Returns true if s is a valid identifier.
     */
    auto const decimal_integer = std::regex{"^(0|[1-9][0-9]*)$"};
    return regex_match(s, decimal_integer);
}
auto is_access_type_specifier(std::string const& s) -> bool {
    return (   s == "%"  // direct access
            or s == "@"  // register indirect access
            or s == "*"  // pointer dereference access
           );
}
}}}}}
