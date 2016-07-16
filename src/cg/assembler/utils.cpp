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

#include <string>
#include <regex>
#include <viua/support/string.h>
#include <viua/cg/assembler/assembler.h>
using namespace std;


regex assembler::utils::getFunctionNameRegex() {
    return regex{"(?:::)?[a-zA-Z_][a-zA-Z0-9_]*(?:::[a-zA-Z_][a-zA-Z0-9_]*)*(?:(/[0-9]*)?)?"};
}

bool assembler::utils::isValidFunctionName(const string& function_name) {
    return regex_match(function_name, getFunctionNameRegex());
}

smatch assembler::utils::matchFunctionName(const string& function_name) {
    smatch parts;
    regex_match(function_name, parts, getFunctionNameRegex());
    return parts;
}

int assembler::utils::getFunctionArity(const string& function_name) {
    int arity = -1;
    auto parts = matchFunctionName(function_name);
    auto sz = parts[1].str().size();
    if (sz > 1) {
        ssub_match a = parts[1];
        arity = stoi(a.str().substr(1)); // cut of the '/' before converting to integer
    } else if (sz == 1) {
        arity = -2;
    }
    return arity;
}

bool assembler::utils::lines::is_function(const string& line) {
    return str::chunk(line) == ".function:";
}

bool assembler::utils::lines::is_block(const string& line) {
    return str::chunk(line) == ".block:";
}

bool assembler::utils::lines::is_function_signature(const string& line) {
    return str::chunk(line) == ".signature:";
}

bool assembler::utils::lines::is_block_signature(const string& line) {
    return str::chunk(line) == ".bsignature:";
}

bool assembler::utils::lines::is_name(const string& line) {
    return str::chunk(line) == ".name:";
}

bool assembler::utils::lines::is_mark(const string& line) {
    return str::chunk(line) == ".mark:";
}

bool assembler::utils::lines::is_info(const string& line) {
    return str::chunk(line) == ".info:";
}

bool assembler::utils::lines::is_end(const string& line) {
    return str::chunk(line) == ".end:";
}

bool assembler::utils::lines::is_main(const string& line) {
    return str::chunk(line) == ".main:";
}

bool assembler::utils::lines::is_directive(const string& line) {
    return (
        is_function(line) or
        is_block(line) or
        is_function_signature(line) or
        is_block_signature(line) or
        is_name(line) or
        is_mark(line) or
        is_info(line) or
        is_end(line) or
        is_main(line) or
        true
    );
}
