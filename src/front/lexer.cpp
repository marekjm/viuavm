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

#include <cstdint>
#include <iostream>
#include <fstream>
#include <utility>
#include <functional>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/version.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


using Token = viua::cg::lex::Token;
using InvalidSyntax = viua::cg::lex::InvalidSyntax;


template<class T> static auto enumerate(const vector<T>& v) -> vector<pair<typename vector<T>::size_type, T>> {
    vector<pair<typename vector<T>::size_type, T>> enumerated_vector;

    typename vector<T>::size_type i = 0;
    for (const auto& each : v) {
        enumerated_vector.emplace_back(i, each);
        ++i;
    }

    return enumerated_vector;
}

static void encode_json(const string& filename, const vector<Token>& tokens) {
    cout << "{";
    cout << str::enquote("file") << ": " << str::enquote(filename) << ',';
    cout << str::enquote("tokens") << ": [";

    const auto limit = tokens.size();
    for (const auto& t : enumerate(tokens)) {
        cout << "{";
        cout << str::enquote("line") << ": " << t.second.line() << ", ";
        cout << str::enquote("character") << ": " << t.second.character() << ", ";
        cout << str::enquote("content") << ": " << str::enquote(str::strencode(t.second.str())) << ", ";
        cout << str::enquote("original") << ": " << str::enquote(str::strencode(t.second.original()));
        cout << '}';
        if (t.first+1 < limit) {
            cout << ", ";
        }
    }

    cout << "]}\n";
}

static bool usage(const char* program, bool show_help, bool show_version, bool verbose) {
    if (show_help or (show_version and verbose)) {
        cout << "Viua VM lexer, version ";
    }
    if (show_help or show_version) {
        cout << VERSION << '.' << MICRO << endl;
    }
    if (show_help) {
        cout << "\nUSAGE:\n";
        cout << "    " << program << " [option...] <infile>\n" << endl;
        cout << "OPTIONS:\n";

        // generic options
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
        // misc options
             << "    " << "    --size               - calculate and display compiled bytecode size\n"
             << "    " << "    --raw                - dump raw token list\n"
             << "    " << "    --ws                 - reduce whitespace and remove comments\n"
             << "    " << "    --dirs               - reduce directives\n"
        ;
    }

    return (show_help or show_version);
}

static string read_file(ifstream& in) {
    ostringstream source_in;
    string line;
    while (getline(in, line)) { source_in << line << '\n'; }

    return source_in.str();
}

static bool DISPLAY_SIZE = false;
static bool DISPLAY_RAW = false;
static bool MANUAL_REDUCING = false;
static bool REDUCE_WHITESPACE = false;
static bool REDUCE_DIRECTIVES = false;

static void display_results(const string& filename, const vector<Token>& tokens) {
    if (DISPLAY_SIZE) {
        try {
            cout << viua::cg::tools::calculate_bytecode_size2(tokens) << endl;
        } catch (const InvalidSyntax& e) {
            cerr << filename << ':' << e.line_number << ':' << e.character_in_line;
            cerr << ": error: invalid syntax: " << str::strencode(e.content) << endl;
        }
        return;
    }

    encode_json(filename, tokens);
}

int main(int argc, char* argv[]) {
    // setup command line arguments vector
    vector<string> args;
    string option;

    string filename(""), compilename("");

    for (int i = 1; i < argc; ++i) {
        option = string(argv[i]);

        if (option == "--help" or option == "-h") {
            SHOW_HELP = true;
            continue;
        } else if (option == "--version" or option == "-V") {
            SHOW_VERSION = true;
            continue;
        } else if (option == "--verbose" or option == "-v") {
            VERBOSE = true;
            continue;
        } else if (option == "--size") {
            DISPLAY_SIZE = true;
            continue;
        } else if (option == "--raw") {
            DISPLAY_RAW = true;
            MANUAL_REDUCING = true;
            continue;
        } else if (option == "--ws") {
            REDUCE_WHITESPACE = true;
            MANUAL_REDUCING = true;
            continue;
        } else if (option == "--dirs") {
            REDUCE_DIRECTIVES = true;
            MANUAL_REDUCING = true;
            continue;
        } else if (str::startswith(option, "-")) {
            cerr << "error: unknown option: " << option << endl;
            return 1;
        }
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) { return 0; }

    if (args.size() == 0) {
        cout << "fatal: no input file" << endl;
        return 1;
    }

    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    filename = args[0];
    if (!filename.size()) {
        cout << "fatal: no file to tokenize" << endl;
        return 1;
    }
    if (!support::env::isfile(filename)) {
        cout << "fatal: could not open file: " << filename << endl;
        return 1;
    }

    ////////////////
    // READ LINES IN
    ifstream in(filename, ios::in | ios::binary);
    if (!in) {
        cout << "fatal: file could not be opened: " << filename << endl;
        return 1;
    }

    string source = read_file(in);

    vector<Token> tokens;
    try {
        tokens = viua::cg::lex::tokenise(source);
        if (not MANUAL_REDUCING) {
            tokens = viua::cg::lex::standardise(viua::cg::lex::cook(tokens));
        }
        if (MANUAL_REDUCING) {
            if (REDUCE_WHITESPACE or REDUCE_DIRECTIVES) {
                tokens = viua::cg::lex::remove_spaces(tokens);
                tokens = viua::cg::lex::remove_comments(tokens);
                tokens = viua::cg::lex::reduce_newlines(tokens);
            }
            if (REDUCE_DIRECTIVES) {
                tokens = reduce_function_directive(tokens);
                tokens = reduce_closure_directive(tokens);
                tokens = reduce_end_directive(tokens);
                tokens = reduce_double_colon(tokens);
                tokens = reduce_function_signatures(tokens);
                tokens = reduce_names(tokens);
                tokens = reduce_signature_directive(tokens);
                tokens = reduce_bsignature_directive(tokens);
                tokens = reduce_block_directive(tokens);
                tokens = reduce_info_directive(tokens);
                tokens = reduce_name_directive(tokens);
                tokens = reduce_main_directive(tokens);
                tokens = reduce_link_directive(tokens);
                tokens = reduce_mark_directive(tokens);
            }
        }
    } catch (const InvalidSyntax& e) {
        string message = e.what();
        cerr << filename << ':' << e.line_number+1 << ':' << e.character_in_line+1 << ": error: " << (message.size() ? message : "invalid syntax") << endl;
        return 1;
    }

    display_results(filename, tokens);

    return 0;
}
