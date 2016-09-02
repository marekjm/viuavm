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

#include <cstdint>
#include <iostream>
#include <fstream>
#include <utility>
#include <functional>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/cg/lex.h>
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
        cout << str::enquote("content") << ": " << str::enquote(str::strencode(t.second.str()));
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
        cout << "    " << program << " [option...] [-o <outfile>] <infile> [<linked-file>...]\n" << endl;
        cout << "OPTIONS:\n";

        // generic options
        cout << "    " << "-V, --version            - show version\n"
             << "    " << "-h, --help               - display this message\n"
         ;
    }

    return (show_help or show_version);
}

template<class T> static bool any(T item, T other) {
    return (item == other);
}
template<class T, class... R> static bool any(T item, T first, R... rest) {
    if (item == first) {
        return true;
    }
    return any(item, rest...);
}

static OPCODE mnemonic_to_opcode(const string& s) {
    OPCODE op = NOP;
    bool found = false;
    for (auto it : OP_NAMES) {
        if (it.second == s) {
            found = true;
            op = it.first;
            break;
        }
    }
    if (not found) {
        throw std::out_of_range("invalid instruction name: " + s);
    }
    return op;
}
static uint64_t calculate_bytecode_size(const vector<Token>& tokens) {
    uint64_t bytes = 0, inc = 0;

    const auto limit = tokens.size();
    for (decltype(tokens.size()) i = 0; i < limit; ++i) {
        Token token = tokens[i];
        if (token.str().substr(0, 1) == ".") {
            while (i < limit and tokens[i].str() != "\n") {
                ++i;
            }
            continue;
        }
        if (token.str() == "\n") {
            continue;
        }
        OPCODE op;
        try {
            op = mnemonic_to_opcode(token.str());
            inc = OP_SIZES.at(token.str());
            if (any(op, ENTER, LINK, WATCHDOG, TAILCALL)) {
                // get second chunk (function, block or module name)
                inc += (tokens.at(i+1).str().size() + 1);
            } else if (any(op, CALL, MSG, PROCESS)) {
                ++i; // skip register index
                if (tokens.at(i+1).str() == "\n") {
                    throw InvalidSyntax(token.line(), token.character(), token.str());
                }
                inc += (tokens.at(i+1).str().size() + 1);
            } else if (any(op, CLOSURE, FUNCTION, CLASS, PROTOTYPE, DERIVE, NEW)) {
                ++i; // skip register index
                if (tokens.at(i+1).str() == "\n") {
                    throw InvalidSyntax(token.line(), token.character(), token.str());
                }
                inc += (tokens.at(i+1).str().size() + 1);
            } else if (op == ATTACH) {
                ++i; // skip register index
                inc += (tokens[++i].str().size() + 1);
                inc += (tokens[++i].str().size() + 1);
            } else if (op == IMPORT) {
                inc += (tokens[++i].str().size() - 2 + 1);
            } else if (op == CATCH) {
                inc += (tokens[++i].str().size() - 2 + 1); // +1: null-terminator, -2: quotes
                inc += (tokens[++i].str().size() + 1);
            } else if (op == STRSTORE) {
                ++i; // skip register index
                inc += (tokens[++i].str().size() - 2 + 1 );
            }
        } catch (const std::out_of_range& e) {
            throw InvalidSyntax(token.line(), token.character(), token.str());
        }

        // skip tokens until "\n" after an instruction has been counted
        while (i < limit and tokens[i].str() != "\n") {
            ++i;
        }

        bytes += inc;
    }

    return bytes;
}

static bool DISPLAY_SIZE = false;

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
        } else if (str::startswith(option, "-")) {
            cout << "error: unknown option: " << option << endl;
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

    ostringstream source_in;
    string line;
    while (getline(in, line)) { source_in << line << '\n'; }

    string source = source_in.str();

    vector<Token> tokens;
    try {
        tokens = viua::cg::lex::standardise(viua::cg::lex::reduce(viua::cg::lex::tokenise(source)));
    } catch (const InvalidSyntax& e) {
        cerr << filename << ':' << e.line_number << ':' << e.character_in_line << ": error: invalid syntax: " << e.content << endl;
        return 1;
    }

    if (DISPLAY_SIZE) {
        try {
            cout << calculate_bytecode_size(tokens) << endl;
        } catch (const InvalidSyntax& e) {
            cerr << filename << ':' << e.line_number << ':' << e.character_in_line;
            cerr << ": error: invalid syntax: " << str::strencode(e.content) << endl;
        }
        return 0;
    }

    encode_json(filename, tokens);

    return 0;
}
