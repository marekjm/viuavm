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
    static auto const identifier = std::regex{"^[a-zA-Z_][a-zA-Z0-9_]*$"};
    return regex_match(s, identifier);
}
auto is_scoped_id(std::string const& s) -> bool {
    static auto const identifier = std::regex{"^[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*$"};
    return regex_match(s, identifier);
}

auto is_binary_integer(std::string const& s) -> bool {
    auto const decimal_integer = std::regex{"^0b[01]+$"};
    return regex_match(s, decimal_integer);
}
auto is_octal_integer(std::string const& s) -> bool {
    auto const decimal_integer = std::regex{"^0o[0-7]+$"};
    return regex_match(s, decimal_integer);
}
auto is_decimal_integer(std::string const& s) -> bool {
    auto const decimal_integer = std::regex{"^(0|[1-9][0-9]*)$"};
    return regex_match(s, decimal_integer);
}
auto is_hexadecimal_integer(std::string const& s) -> bool {
    auto const decimal_integer = std::regex{"^0x[0-9a-f]+$"};
    return regex_match(s, decimal_integer);
}

auto is_access_type_specifier(std::string const& s) -> bool {
    return (   s == "%"  // direct access
            or s == "@"  // register indirect access
            or s == "*"  // pointer dereference access
           );
}
auto is_register_set_name(std::string const& s) -> bool {
    return (   s == "local"
            or s == "static"
            or s == "global"
            or s == "arguments"
            or s == "parameters"
            or s == "closure_local"
           );
}

auto is_quoted_text(std::string const& s, std::string::value_type const c) -> bool {
    if (s.empty()) {
        return false;
    }
    if (s.size() < 2) {
        return false;
    }
    return (s.front() == c and s.back() == c);
}
auto is_float(std::string const& s) -> bool {
    auto const decimal_integer = std::regex{"^-?(0|[1-9][0-9]*)\\.[0-9]+$"};
    return regex_match(s, decimal_integer);
}
auto is_default(std::string const& s) -> bool {
    return (s == "default");
}

auto is_boolean_literal(std::string const& s) -> bool {
    return (s == "true" or s == "false");
}
auto is_timeout_literal(std::string const& s) -> bool {
    if (s == "infinity") {
        return true;
    }

    auto const timeout_literal = std::regex{"^(0|[1-9][0-9]*)m?s$"};
    return regex_match(s, timeout_literal);
}
}}}}}
