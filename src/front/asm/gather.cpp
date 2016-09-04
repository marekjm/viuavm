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

#include <iostream>
#include <viua/support/string.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
using namespace std;


int gatherFunctions(invocables_t* invocables, const vector<viua::cg::lex::Token>& tokens) {
    ///////////////////////////////////////////
    // GATHER FUNCTION NAMES AND SIGNATURES
    //
    // SIGNATURES ARE USED WITH DYNAMIC LINKING
    // AS ASSEMBLER WOULD COMPLAIN ABOUT
    // CALLS TO UNDEFINED FUNCTIONS
    try {
        invocables->names = assembler::ce::getFunctionNames(tokens);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }

    try {
        invocables->signatures = assembler::ce::getSignatures(tokens);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }

    ///////////////////////////////
    // GATHER FUNCTIONS' CODE LINES
    try {
         invocables->bodies = assembler::ce::getInvokables("function", tokens);
         for (const auto& each : assembler::ce::getInvokables("closure", tokens)) {
            invocables->bodies[each.first] = each.second;
         }
         invocables->tokens = assembler::ce::getInvokablesTokenBodies("function", tokens);
         for (const auto& each : assembler::ce::getInvokablesTokenBodies("closure", tokens)) {
            invocables->tokens[each.first] = each.second;
         }
    } catch (const string& e) {
        cout << "error: function gathering failed: " << e << endl;
        return 1;
    }

    return 0;
}

int gatherBlocks(invocables_t* invocables, const vector<viua::cg::lex::Token>& tokens) {
    /////////////////////
    // GATHER BLOCK NAMES
    try {
        invocables->names = assembler::ce::getBlockNames(tokens);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }
    try {
        invocables->signatures = assembler::ce::getBlockSignatures(tokens);
    } catch (const string& e) {
        cout << "fatal: " << e << endl;
        return 1;
    }

    ///////////////////////////////
    // GATHER BLOCK CODE LINES
    try {
         invocables->bodies = assembler::ce::getInvokables("block", tokens);
         invocables->tokens = assembler::ce::getInvokablesTokenBodies("block", tokens);
    } catch (const string& e) {
        cout << "error: block gathering failed: " << e << endl;
        return 1;
    }

    return 0;
}

map<string, string> gatherMetaInformation(const vector<string>& ilines) {
    map<string, string> meta_information;

    string line;
    for (std::remove_reference<decltype(ilines)>::type::size_type i = 0; i < ilines.size(); ++i) {
        line = ilines[i];
        if (assembler::utils::lines::is_info(line)) {
            line = str::lstrip(line.substr(6));

            string key, value;
            key = str::chunk(line);
            line = str::lstrip(str::sub(line, key.size()));
            value = str::extract(line);
            meta_information[key] = str::strdecode(value.substr(1, value.size()-2));
        }
    }

    return meta_information;
}
