/*
 *  Copyright (C) 2017, 2018 Marek Marecki
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
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <viua/assembler/frontend/parser.h>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/version.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP    = false;
bool SHOW_VERSION = false;
bool VERBOSE      = false;

bool AS_LIB = false;


using namespace viua::assembler::frontend::parser;
using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::Token;
using viua::cg::lex::TracedSyntaxError;


template<class T>
static auto enumerate(const vector<T>& v)
    -> vector<pair<typename vector<T>::size_type, T>> {
    vector<pair<typename vector<T>::size_type, T>> enumerated_vector;

    typename vector<T>::size_type i = 0;
    for (const auto& each : v) {
        enumerated_vector.emplace_back(i, each);
        ++i;
    }

    return enumerated_vector;
}

static bool usage(const char* program, bool show_help, bool show_version,
                  bool verbose) {
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
        cout << "    "
             << "-V, --version            - show version\n"
             << "    "
             << "-h, --help               - display this message\n"
             << "    ";

        // compilation options
        cout << "-c, --lib                - assemble as a library\n";
    }

    return (show_help or show_version);
}

static string read_file(ifstream& in) {
    ostringstream source_in;
    string line;
    while (getline(in, line)) {
        source_in << line << '\n';
    }

    return source_in.str();
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
        } else if (option == "--lib" or option == "-c") {
            AS_LIB = true;
            continue;
        } else if (str::startswith(option, "-")) {
            cerr << "error: unknown option: " << option << endl;
            return 1;
        }
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) {
        return 0;
    }

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
    if (!support::env::is_file(filename)) {
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

    auto raw_tokens = viua::cg::lex::tokenise(source);
    vector<Token> tokens;
    vector<Token> normalised_tokens;

    try {
        tokens            = viua::cg::lex::cook(raw_tokens);
        normalised_tokens = normalise(tokens);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    try {
        auto parsed_source =
            viua::assembler::frontend::parser::parse(normalised_tokens);
        parsed_source.as_library = AS_LIB;
        viua::assembler::frontend::static_analyser::verify(parsed_source);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    return 0;
}
