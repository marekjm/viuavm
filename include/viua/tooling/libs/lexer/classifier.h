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

#ifndef VIUA_TOOLING_LIBS_LEXER_CLASSIFIER_H
#define VIUA_TOOLING_LIBS_LEXER_CLASSIFIER_H

#include <string>

namespace viua {
namespace tooling {
namespace libs {
namespace lexer {
namespace classifier {
auto is_id(std::string const&) -> bool;
auto is_scoped_id(std::string const&) -> bool;

auto is_binary_integer(std::string const&) -> bool;
auto is_octal_integer(std::string const&) -> bool;
auto is_decimal_integer(std::string const&) -> bool;
auto is_hexadecimal_integer(std::string const&) -> bool;

auto is_access_type_specifier(std::string const&) -> bool;
auto is_register_set_name(std::string const&) -> bool;

auto is_quoted_text(std::string const&, std::string::value_type const = '"') -> bool;
auto is_float(std::string const&) -> bool;
auto is_default(std::string const&) -> bool;
auto is_void(std::string const&) -> bool;

auto is_boolean_literal(std::string const&) -> bool;
auto is_timeout_literal(std::string const&) -> bool;
}}}}}

#endif
