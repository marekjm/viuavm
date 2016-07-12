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
