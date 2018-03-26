/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>
#include <viua/support/string.h>
using namespace std;


using ErrorReport = pair<unsigned, string>;
using Token       = viua::cg::lex::Token;


static bool is_defined(string function_name,
                       const vector<string>& function_names,
                       const vector<string>& function_signatures) {
    bool is_undefined =
        (find(function_names.begin(), function_names.end(), function_name) ==
         function_names.end());
    // if function is undefined, check if we got a signature for it
    if (is_undefined) {
        is_undefined = (find(function_signatures.begin(),
                             function_signatures.end(),
                             function_name) == function_signatures.end());
    }
    return (not is_undefined);
}
void assembler::verify::function_calls_are_defined(
    const vector<Token>& tokens,
    const vector<string>& function_names,
    const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        auto token = tokens.at(i);
        if (not(token == "call" or token == "process" or token == "watchdog" or
                token == "tailcall" or token == "defer")) {
            continue;
        }

        if (token == "tailcall" or token == "defer") {
            auto function_name = tokens.at(i + 1);
            if (function_name.str().at(0) != '*' and
                function_name.str().at(0) != '%') {
                if (not is_defined(
                        function_name, function_names, function_signatures)) {
                    throw viua::cg::lex::InvalidSyntax(
                        function_name,
                        (string(token == "tailcall" ? "tail call to"
                                                    : "deferred") +
                         " undefined function " + function_name.str()));
                }
            }
        } else if (token == "watchdog") {
            auto function_name = tokens.at(i + 1);
            if (not is_defined(
                    function_name, function_names, function_signatures)) {
                throw viua::cg::lex::InvalidSyntax(
                    function_name,
                    "watchdog from undefined function " + function_name.str());
            }
        } else if (token == "call" or token == "process") {
            Token function_name = tokens.at(i + 2);
            if (tokens.at(i + 1) != "void") {
                function_name = tokens.at(i + 3);
            }
            if (function_name.str().at(0) != '*' and
                function_name.str().at(0) != '%') {
                if (not is_defined(
                        function_name, function_names, function_signatures)) {
                    throw viua::cg::lex::InvalidSyntax(
                        function_name,
                        (string(token == "call" ? "call to" : "process from") +
                         " undefined function " + function_name.str()));
                }
            }
        }
    }
}

void assembler::verify::callable_creations(
    const vector<Token>& tokens,
    const vector<string>& function_names,
    const vector<string>& function_signatures) {
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0;
         i < tokens.size();
         ++i) {
        if (not(tokens.at(i) == "closure" or tokens.at(i) == "function")) {
            // skip while lines to avoid triggering the check by registers named
            // 'function' or 'closure'
            while (i < tokens.size() and tokens.at(i) != "\n") {
                ++i;
            }
            continue;
        }

        string function = tokens.at(i + 3);

        if (not assembler::utils::is_valid_function_name(function)) {
            // we don't have a valid function name here, let's defer to SA to
            // catch the error (?)
            continue;
        }

        bool is_undefined =
            (find(function_names.begin(), function_names.end(), function) ==
             function_names.end());
        // if function is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(function_signatures.begin(),
                                 function_signatures.end(),
                                 function) == function_signatures.end());
        }

        if (is_undefined) {
            viua::cg::lex::InvalidSyntax e(
                tokens.at(i),
                (tokens.at(i).str() + " from undefined function: " + function));
            e.add(tokens.at(i + 2));
            throw e;
        }
        i += 2;
    }
}
