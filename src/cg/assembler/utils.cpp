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
using namespace std;


regex assembler::utils::get_function_name_regex() {
    // FIXME function names *MUST* end with '/' to avoid ambiguity when
    // determining whether a token is a function name, or a label
    return regex{
        "(?:::)?[a-zA-Z_][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*/([0-9]+)?"};
}

bool assembler::utils::is_valid_function_name(const std::string& function_name) {
    return regex_match(function_name, get_function_name_regex());
}

smatch assembler::utils::match_function_name(const std::string& function_name) {
    smatch parts;
    regex_match(function_name, parts, get_function_name_regex());
    return parts;
}

int assembler::utils::get_function_arity(const std::string& function_name) {
    int arity  = -1;
    auto parts = match_function_name(function_name);
    if (auto a = parts[1].str(); a.size()) {
        arity = stoi(a);
    }
    return arity;
}

bool assembler::utils::lines::is_function(const std::string& line) {
    return str::chunk(line) == ".function:";
}

bool assembler::utils::lines::is_closure(const std::string& line) {
    return str::chunk(line) == ".closure:";
}

bool assembler::utils::lines::is_block(const std::string& line) {
    return str::chunk(line) == ".block:";
}

bool assembler::utils::lines::is_function_signature(const std::string& line) {
    return str::chunk(line) == ".signature:";
}

bool assembler::utils::lines::is_block_signature(const std::string& line) {
    return str::chunk(line) == ".bsignature:";
}

bool assembler::utils::lines::is_name(const std::string& line) {
    return str::chunk(line) == ".name:";
}

bool assembler::utils::lines::is_mark(const std::string& line) {
    return str::chunk(line) == ".mark:";
}

bool assembler::utils::lines::is_info(const std::string& line) {
    return str::chunk(line) == ".info:";
}

bool assembler::utils::lines::is_end(const std::string& line) {
    return str::chunk(line) == ".end";
}

bool assembler::utils::lines::is_import(const std::string& line) {
    return str::chunk(line) == ".import:";
}

bool assembler::utils::lines::is_directive(const std::string& line) {
    return (is_function(line) or is_block(line) or is_function_signature(line)
            or is_block_signature(line) or is_name(line) or is_mark(line)
            or is_info(line) or is_end(line) or is_import(line)
            or is_closure(line) or line == ".unused:" or false);
}

std::string assembler::utils::lines::make_function_signature(const std::string& name) {
    return (".signature: " + name);
}

std::string assembler::utils::lines::make_block_signature(const std::string& name) {
    return (".bsignature: " + name);
}

std::string assembler::utils::lines::make_info(const std::string& key,
                                          const std::string& value) {
    return (".info: " + key + ' ' + str::enquote(str::strencode(value)));
}
