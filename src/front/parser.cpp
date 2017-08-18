/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>
#include <viua/assembler/frontend/parser.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/version.h>
using namespace std;


// MISC FLAGS
bool SHOW_HELP = false;
bool SHOW_VERSION = false;
bool VERBOSE = false;


using namespace viua::assembler::frontend::parser;
using Token = viua::cg::lex::Token;
using InvalidSyntax = viua::cg::lex::InvalidSyntax;


template<class T>
static auto enumerate(const vector<T>& v) -> vector<pair<typename vector<T>::size_type, T>> {
    vector<pair<typename vector<T>::size_type, T>> enumerated_vector;

    typename vector<T>::size_type i = 0;
    for (const auto& each : v) {
        enumerated_vector.emplace_back(i, each);
        ++i;
    }

    return enumerated_vector;
}


string send_control_seq(const string& mode) {
    static auto is_terminal = isatty(1);
    static string env_color_flag{getenv("VIUAVM_ASM_COLOUR") ? getenv("VIUAVM_ASM_COLOUR") : "default"};

    bool colorise = is_terminal;
    if (env_color_flag == "default") {
        // do nothing; the default is to colorise when printing to teminal and
        // do not colorise otherwise
    } else if (env_color_flag == "never") {
        colorise = false;
    } else if (env_color_flag == "always") {
        colorise = true;
    } else {
        // unknown value, do nothing
    }

    if (colorise) {
        return mode;
    }
    return "";
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
        cout << "    "
             << "-V, --version            - show version\n"
             << "    "
             << "-h, --help               - display this message\n"
             // misc options
             << "    "
             << "    --size               - calculate and display compiled bytecode size\n"
             << "    "
             << "    --raw                - dump raw token list\n"
             << "    "
             << "    --ws                 - reduce whitespace and remove comments\n"
             << "    "
             << "    --dirs               - reduce directives\n";
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

static void underline_error_token(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i,
                                  const viua::cg::lex::InvalidSyntax& error) {
    cout << "     ";

    auto len = str::stringify((error.line() + 1), false).size();
    while (len--) {
        cout << ' ';
    }
    cout << ' ';

    while (i < tokens.size()) {
        const auto& each = tokens.at(i++);
        bool match = error.match(each);

        if (match) {
            cout << send_control_seq(COLOR_FG_RED_1);
        }

        char c = (match ? '^' : ' ');
        len = each.str().size();
        while (len--) {
            cout << c;
        }

        if (match) {
            cout << send_control_seq(ATTR_RESET);
        }

        if (each == "\n") {
            break;
        }
    }

    cout << '\n';
}
static auto display_error_line(const vector<viua::cg::lex::Token>& tokens,
                               const viua::cg::lex::InvalidSyntax& error, decltype(tokens.size()) i)
    -> decltype(i) {
    const auto token_line = tokens.at(i).line();

    cout << send_control_seq(COLOR_FG_RED);
    cout << ">>>>";  // message indent, ">>>>" on error line
    cout << ' ';

    cout << send_control_seq(COLOR_FG_YELLOW);
    cout << token_line + 1;
    cout << ' ';

    auto original_i = i;

    cout << send_control_seq(COLOR_FG_WHITE);
    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        bool highlighted = false;
        if (error.match(tokens.at(i))) {
            cout << send_control_seq(COLOR_FG_ORANGE_RED_1);
            highlighted = true;
        }
        cout << tokens.at(i++).str();
        if (highlighted) {
            cout << send_control_seq(COLOR_FG_WHITE);
        }
    }

    cout << send_control_seq(ATTR_RESET);

    underline_error_token(tokens, original_i, error);

    return i;
}
static auto display_context_line(const vector<viua::cg::lex::Token>& tokens,
                                 const viua::cg::lex::InvalidSyntax&, decltype(tokens.size()) i)
    -> decltype(i) {
    const auto token_line = tokens.at(i).line();

    cout << "    ";  // message indent, ">>>>" on error line
    cout << ' ';
    cout << token_line + 1;
    cout << ' ';

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        cout << tokens.at(i++).str();
    }

    return i;
}
static void display_error_header(const viua::cg::lex::InvalidSyntax& error, const string& filename) {
    cout << send_control_seq(COLOR_FG_WHITE) << filename << ':' << error.line() + 1 << ':'
         << error.character() + 1 << ':' << send_control_seq(ATTR_RESET) << ' ';
    cout << send_control_seq(COLOR_FG_RED) << "error" << send_control_seq(ATTR_RESET) << ": " << error.what()
         << endl;
}
static void display_error_location(const vector<viua::cg::lex::Token>& tokens,
                                   const viua::cg::lex::InvalidSyntax error) {
    const unsigned context_lines = 2;
    decltype(error.line()) context_before = 0, context_after = (error.line() + context_lines);
    if (error.line() >= context_lines) {
        context_before = (error.line() - context_lines);
    }

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size();) {
        if (tokens.at(i).line() > context_after) {
            break;
        }
        if (tokens.at(i).line() >= context_before) {
            if (tokens.at(i).line() == error.line()) {
                i = display_error_line(tokens, error, i);
            } else {
                i = display_context_line(tokens, error, i);
            }
            continue;
        }
        ++i;
    }
}
static void display_error_in_context(const vector<viua::cg::lex::Token>& tokens,
                                     const viua::cg::lex::InvalidSyntax error, const string& filename) {
    display_error_header(error, filename);
    cout << "\n";
    display_error_location(tokens, error);
}
static void display_error_in_context(const vector<viua::cg::lex::Token>& tokens,
                                     const viua::cg::lex::TracedSyntaxError error, const string& filename) {
    for (auto const& e : error.errors) {
        display_error_in_context(tokens, e, filename);
        cout << "\n";
    }
}


static auto display_result(const ParsedSource&) -> void {}


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

    auto raw_tokens = viua::cg::lex::tokenise(source);
    vector<Token> tokens;
    vector<Token> normalised_tokens;

    try {
        tokens = viua::cg::lex::cook(raw_tokens);
        normalised_tokens = normalise(tokens);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    try {
        auto parsed_source = viua::assembler::frontend::parser::parse(normalised_tokens);
        display_result(parsed_source);
    } catch (const viua::cg::lex::InvalidSyntax& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    } catch (const viua::cg::lex::TracedSyntaxError& e) {
        display_error_in_context(raw_tokens, e, filename);
        return 1;
    }

    return 0;
}
