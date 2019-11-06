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


// MISC FLAGS
bool SHOW_HELP    = false;
bool SHOW_VERSION = false;
bool VERBOSE      = false;

bool AS_LIB = false;


using namespace viua::assembler::frontend::parser;
using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;


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
        std::cout << "    "
                  << "-V, --version            - show version\n"
                  << "    "
                  << "-h, --help               - display this message\n"
                  << "    ";

        // compilation options
        std::cout << "-c, --lib                - assemble as a library\n";
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
        else if (option == "--lib" or option == "-c") {
            AS_LIB = true;
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

    auto raw_tokens        = viua::cg::lex::tokenise(source);
    auto tokens            = std::vector<Token>{};
    auto normalised_tokens = std::vector<Token>{};

    try {
        tokens            = viua::cg::lex::cook(raw_tokens);
        normalised_tokens = normalise(tokens);
    }
    catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }
    catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    try {
        auto parsed_source =
            viua::assembler::frontend::parser::parse(normalised_tokens);
        parsed_source.as_library = AS_LIB;
        viua::assembler::frontend::static_analyser::verify(parsed_source);
    }
    catch (viua::cg::lex::Invalid_syntax const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }
    catch (viua::cg::lex::Traced_syntax_error const& e) {
        viua::assembler::util::pretty_printer::display_error_in_context(
            raw_tokens, e, filename);
        return 1;
    }

    return 0;
}
