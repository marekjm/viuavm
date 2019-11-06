/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
#include <utility>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>
#include <viua/cg/tools.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/version.h>


// MISC FLAGS
bool SHOW_HELP    = false;
bool SHOW_VERSION = false;
bool VERBOSE      = false;


using Token          = viua::cg::lex::Token;
using Invalid_syntax = viua::cg::lex::Invalid_syntax;


template<class T>
static auto enumerate(std::vector<T> const& v)
    -> std::vector<std::pair<typename std::vector<T>::size_type, T>>
{
    auto enumerated_vector =
        std::vector<std::pair<typename std::vector<T>::size_type, T>>{};

    typename std::vector<T>::size_type i = 0;
    for (auto const& each : v) {
        enumerated_vector.emplace_back(i, each);
        ++i;
    }

    return enumerated_vector;
}

static void encode_json(std::string const& filename,
                        std::vector<Token> const& tokens)
{
    std::cout << "{";
    std::cout << str::enquote("file") << ": " << str::enquote(filename) << ',';
    std::cout << str::enquote("tokens") << ": [";

    const auto limit = tokens.size();
    for (auto const& t : enumerate(tokens)) {
        std::cout << "{";
        std::cout << str::enquote("line") << ": " << t.second.line() << ", ";
        std::cout << str::enquote("character") << ": " << t.second.character()
                  << ", ";
        std::cout << str::enquote("content") << ": "
                  << str::enquote(str::strencode(t.second.str())) << ", ";
        std::cout << str::enquote("original") << ": "
                  << str::enquote(str::strencode(t.second.original()));
        std::cout << '}';
        if (t.first + 1 < limit) {
            std::cout << ", ";
        }
    }

    std::cout << "]}\n";
}

static bool usage(const char* program,
                  bool show_help,
                  bool show_version,
                  bool verbose)
{
    if (show_help or (show_version and verbose)) {
        std::cout << "Viua VM lexer, version ";
    }
    if (show_help or show_version) {
        std::cout << VERSION << '.' << MICRO << std::endl;
    }
    if (show_help) {
        std::cout << "\nUSAGE:\n";
        std::cout << "    " << program << " [option...] <infile>\n"
                  << std::endl;
        std::cout << "OPTIONS:\n";

        // generic options
        std::cout
            << "    "
            << "-V, --version            - show version\n"
            << "    "
            << "-h, --help               - display this message\n"
            // misc options
            << "    "
            << "    --size               - calculate and display compiled "
               "bytecode size\n"
            << "    "
            << "    --raw                - dump raw token list\n"
            << "    "
            << "    --ws                 - reduce whitespace and remove "
               "comments\n"
            << "    "
            << "    --dirs               - reduce directives\n";
    }

    return (show_help or show_version);
}

static std::string read_file(std::ifstream& in)
{
    std::ostringstream source_in;
    auto line = std::string{};
    while (std::getline(in, line)) {
        source_in << line << '\n';
    }

    return source_in.str();
}

static bool DISPLAY_SIZE      = false;
static bool DISPLAY_RAW       = false;
static bool MANUAL_REDUCING   = false;
static bool REDUCE_WHITESPACE = false;
static bool REDUCE_DIRECTIVES = false;

static void display_results(std::string const& filename,
                            std::vector<Token> const& tokens)
{
    if (DISPLAY_SIZE) {
        try {
            std::cout << viua::cg::tools::calculate_bytecode_size2(tokens)
                      << std::endl;
        }
        catch (Invalid_syntax const& e) {
            std::cerr << filename << ':' << e.line_number << ':'
                      << e.character_in_line;
            std::cerr << ": error: invalid syntax: "
                      << str::strencode(e.content) << std::endl;
        }
        return;
    }

    encode_json(filename, tokens);
}

int main(int argc, char* argv[])
{
    // setup command line arguments vector
    auto args   = std::vector<std::string>{};
    auto option = std::string{};

    std::string filename(""), compilename("");

    for (int i = 1; i < argc; ++i) {
        option = std::string(argv[i]);

        if (option == "--help" or option == "-h") {
            SHOW_HELP = true;
            continue;
        }
        else if (option == "--version" or option == "-V") {
            SHOW_VERSION = true;
            continue;
        }
        else if (option == "--verbose" or option == "-v") {
            VERBOSE = true;
            continue;
        }
        else if (option == "--size") {
            DISPLAY_SIZE = true;
            continue;
        }
        else if (option == "--raw") {
            DISPLAY_RAW     = true;
            MANUAL_REDUCING = true;
            continue;
        }
        else if (option == "--ws") {
            REDUCE_WHITESPACE = true;
            MANUAL_REDUCING   = true;
            continue;
        }
        else if (option == "--dirs") {
            REDUCE_DIRECTIVES = true;
            MANUAL_REDUCING   = true;
            continue;
        }
        else if (str::startswith(option, "-")) {
            std::cerr << "error: unknown option: " << option << std::endl;
            return 1;
        }
        args.emplace_back(argv[i]);
    }

    if (usage(argv[0], SHOW_HELP, SHOW_VERSION, VERBOSE)) {
        return 0;
    }

    if (args.size() == 0) {
        std::cerr << "fatal: no input file" << std::endl;
        return 1;
    }

    ////////////////////////////////
    // FIND FILENAME AND COMPILENAME
    filename = args[0];
    if (!filename.size()) {
        std::cerr << "fatal: no file to tokenize" << std::endl;
        return 1;
    }
    if (!viua::support::env::is_file(filename)) {
        std::cerr << "fatal: could not open file: " << filename << std::endl;
        return 1;
    }

    ////////////////
    // READ LINES IN
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "fatal: file could not be opened: " << filename
                  << std::endl;
        return 1;
    }

    auto const source = read_file(in);

    auto tokens = std::vector<Token>{};
    try {
        tokens = viua::cg::lex::tokenise(source);
        if (not MANUAL_REDUCING) {
            tokens = viua::cg::lex::normalise(viua::cg::lex::cook(tokens));
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
                tokens = reduce_import_directive(tokens);
                tokens = reduce_mark_directive(tokens);
            }
        }
    }
    catch (Invalid_syntax const& e) {
        auto const message = std::string{e.what()};
        std::cerr << filename << ':' << e.line_number + 1 << ':'
                  << e.character_in_line + 1 << ": error: "
                  << (message.size() ? message : "invalid syntax") << std::endl;
        return 1;
    }

    display_results(filename, tokens);

    return 0;
}
