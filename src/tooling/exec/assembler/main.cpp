/*
 *  Copyright (C) 2018 Marek Marecki
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
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <viua/version.h>
#include <viua/tooling/errors/compile_time.h>
#include <viua/util/filesystem.h>
#include <viua/util/string/escape_sequences.h>
#include <viua/tooling/libs/lexer/tokenise.h>

std::string const OPTION_HELP_LONG = "--help";
std::string const OPTION_HELP_SHORT = "-h";
std::string const OPTION_VERSION = "--version";

static auto usage(std::vector<std::string> const& args) -> bool {
    auto help_screen = (args.size() == 1);
    auto version_screen = false;
    if (std::find(args.begin(), args.end(), OPTION_HELP_LONG) != args.end()) {
        help_screen = true;
    }
    if (std::find(args.begin(), args.end(), OPTION_HELP_SHORT) != args.end()) {
        help_screen = true;
    }
    if (std::find(args.begin(), args.end(), OPTION_VERSION) != args.end()) {
        version_screen = true;
    }

    if (version_screen) {
        std::cout << "Viua VM assembler version " << VERSION << '.' << MICRO << std::endl;
    }
    if (help_screen) {
        if (version_screen) {
            std::cout << '\n';
        }
        std::cout << "SYNOPSIS\n";
        std::cout << "    " << args.at(0) << " [<option>...] [-o <output>] <source>.asm [<module>...]\n";
        std::cout << '\n';
        std::cout << "    <option> is any option that is accepted by the assembler.\n";
        std::cout << "    <output> is the file to which the assembled bytecode will be written."
                     " The default is to write output to `a.out` file.\n";
        std::cout << "    <source> is the name of the file containing the Viua VM assembly language"
                     " program to be assembled.\n";
        std::cout << "    <module> is a module that is to be statically linked to the currently assembled"
                     " file.\n";

        std::cout << '\n';

        std::cout << "OPTIONS\n";
        std::cout << "    -h, --help        - display help screen\n";
        std::cout << "        --version     - display version\n";
        std::cout << "    -o, --out <file>  - write output to <file>; the default is to write to `a.out`\n";
        std::cout << "    -c, --lib         - assemble as a linkable module; the default is to assemble an"
                     " executable\n";
        std::cout << "    -C, --verify      - perform static analysis, but do not output bytecode\n";
        std::cout << "        --no-sa       - disable static analysis (useful in case of false positives"
                     " being thrown by the SA engine)\n";

        std::cout << '\n';

        std::cout << "COPYRIGHT\n";
        std::cout << "    Copyright (C) 2018 Marek Marecki\n";
        std::cout << "    License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n";
        std::cout << "    This is free software: you are free to change and redistribute it.\n";
        std::cout << "    There is NO WARRANTY, to the extent permitted by law.\n";
    }

    return (help_screen or version_screen);
}

static auto make_args(int const argc, char const* const argv[]) -> std::vector<std::string> {
    auto args = std::vector<std::string>{};
    std::copy_n(argv, argc, std::back_inserter(args));
    return args;
}

struct Parsed_args {
    bool verbose = false;
    bool enabled_sa = true;
    bool sa_only = false;
    bool linkable_module = false;

    std::string output_file = "a.out";
    std::string input_file;

    std::vector<std::string> linked_modules;

    std::map<std::string, std::string> options;
};

static auto parse_args(std::vector<std::string> const& args) -> Parsed_args {
    auto parsed = Parsed_args{};

    /*
     * Parse options at first. Stop parsing on first token that does not start with
     * '--', or is '--'.
     */
    auto i = std::remove_reference_t<decltype(args)>::size_type{1};
    for (; i < args.size(); ++i) {
        auto const& arg = args.at(i);

        if (arg == "-o" or arg == "--output") {
            parsed.output_file = args.at(++i);
        } else if (arg == "--verbose") {
            parsed.verbose = true;
        } else if (arg == "-c") {
            parsed.linkable_module = true;
        } else if (arg == "-C" or arg == "--verify") {
            parsed.sa_only = true;
        } else if (arg == "-h" or arg == "--help") {
            // do nothing
        } else if (arg == "--version") {
            // do nothing
        } else if (arg == "--") {
            ++i;
            break;
        } else if (arg.find("--") == 0) {
            viua::tooling::errors::compile_time::display_error_and_exit(
                viua::tooling::errors::compile_time::Compile_time_error::Unknown_option
                , arg
            );
        } else if (arg.find("--") != 0) {
            break;
        }
    }

    if (i == args.size()) {
        viua::tooling::errors::compile_time::display_error_and_exit(
            viua::tooling::errors::compile_time::Compile_time_error::No_input_file
        );
    }
    parsed.input_file = args.at(i);
    if (parsed.input_file.size() == 0) {
        viua::tooling::errors::compile_time::display_error_and_exit(
            viua::tooling::errors::compile_time::Compile_time_error::No_input_file
        );
    }

    if (parsed.sa_only) {
        parsed.output_file = "/dev/null";
    }

    return parsed;
}

static auto read_file(std::string const& path) -> std::string {
    std::ifstream in(path, std::ios::in | std::ios::binary);

    std::ostringstream source_in;
    auto line = std::string{};
    while (std::getline(in, line)) {
        source_in << line << '\n';
    }

    return source_in.str();
}

#ifdef JSON_TOKEN_DUMP
static auto to_json(viua::tooling::libs::lexer::Token const& token) -> std::string {
    auto o = std::ostringstream{};

    o << '{';
    o << std::quoted("line") << ':' << token.line();
    o << ',';
    o << std::quoted("character") << ':' << token.character();
    o << ',';
    o << std::quoted("content") << ':' << std::quoted(token.str());
    o << ',';
    o << std::quoted("original") << ':' << std::quoted(token.original());
    o << '}';

    return o.str();
}
static auto to_json(std::vector<viua::tooling::libs::lexer::Token> const& tokens) -> std::string {
    auto o = std::ostringstream{};

    o << '[' << '\n';
    o << ' ' << to_json(tokens.at(0)) << '\n';
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1}; i < tokens.size(); ++i) {
        o << ',' << to_json(tokens.at(i)) << '\n';
    }
    o << ']';

    return o.str();
}
#endif

auto main(int argc, char* argv[]) -> int {
    auto const args = make_args(argc, argv);
    if (usage(args)) {
        return 0;
    }

    auto const parsed_args = parse_args(args);

    if (parsed_args.verbose) {
        std::cerr << "input file: " << parsed_args.input_file << std::endl;
        std::cerr << "output file: " << parsed_args.output_file << std::endl;
    }

    if (not viua::util::filesystem::is_file(parsed_args.input_file)) {
        viua::tooling::errors::compile_time::display_error_and_exit(
            viua::tooling::errors::compile_time::Compile_time_error::Not_a_file
            , parsed_args.input_file
        );
    }

    auto const source = read_file(parsed_args.input_file);
    auto const raw_tokens = viua::tooling::libs::lexer::tokenise(source);

    using viua::tooling::libs::lexer::strip_spaces;
    auto const tokens = strip_spaces(raw_tokens);

#ifdef JSON_TOKEN_DUMP
    std::cerr << tokens.size() << std::endl;
    std::cerr << to_json(tokens) << std::endl;
#endif

    return 0;
}
