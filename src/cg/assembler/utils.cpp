/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#include <regex>
#include <string>

#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>


std::regex assembler::utils::get_function_name_regex()
{
    // FIXME function names *MUST* end with '/' to avoid ambiguity when
    // determining whether a token is a function name, or a label
    return std::regex{
        "(?:::)?[a-zA-Z_][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*/([0-9]+)?"};
}

bool assembler::utils::is_valid_function_name(std::string const& function_name)
{
    return std::regex_match(function_name, get_function_name_regex());
}

std::smatch assembler::utils::match_function_name(
    std::string const& function_name)
{
    std::smatch parts;
    std::regex_match(function_name, parts, get_function_name_regex());
    return parts;
}

int assembler::utils::get_function_arity(std::string const& function_name)
{
    int arity  = -1;
    auto parts = match_function_name(function_name);
    if (auto a = parts[1].str(); a.size()) {
        arity = stoi(a);
    }
    return arity;
}

bool assembler::utils::lines::is_function(std::string const& line)
{
    return str::chunk(line) == ".function:";
}

bool assembler::utils::lines::is_closure(std::string const& line)
{
    return str::chunk(line) == ".closure:";
}

bool assembler::utils::lines::is_block(std::string const& line)
{
    return str::chunk(line) == ".block:";
}

bool assembler::utils::lines::is_function_signature(std::string const& line)
{
    return str::chunk(line) == ".signature:";
}

bool assembler::utils::lines::is_block_signature(std::string const& line)
{
    return str::chunk(line) == ".bsignature:";
}

bool assembler::utils::lines::is_name(std::string const& line)
{
    return str::chunk(line) == ".name:";
}

bool assembler::utils::lines::is_mark(std::string const& line)
{
    return str::chunk(line) == ".mark:";
}

bool assembler::utils::lines::is_info(std::string const& line)
{
    return str::chunk(line) == ".info:";
}

bool assembler::utils::lines::is_end(std::string const& line)
{
    return str::chunk(line) == ".end";
}

bool assembler::utils::lines::is_import(std::string const& line)
{
    return str::chunk(line) == ".import:";
}

bool assembler::utils::lines::is_directive(std::string const& line)
{
    return (is_function(line) or is_block(line) or is_function_signature(line)
            or is_block_signature(line) or is_name(line) or is_mark(line)
            or is_info(line) or is_end(line) or is_import(line)
            or is_closure(line) or line == ".unused:" or false);
}

std::string assembler::utils::lines::make_function_signature(
    std::string const& name)
{
    return (".signature: " + name);
}

std::string assembler::utils::lines::make_block_signature(
    std::string const& name)
{
    return (".bsignature: " + name);
}

std::string assembler::utils::lines::make_info(std::string const& key,
                                               std::string const& value)
{
    return (".info: " + key + ' ' + str::enquote(str::strencode(value)));
}
