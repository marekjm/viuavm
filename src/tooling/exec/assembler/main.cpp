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
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <viua/version.h>
#include <viua/tooling/errors/compile_time.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/util/filesystem.h>
#include <viua/util/string/escape_sequences.h>
#include <viua/util/string/ops.h>
#include <viua/tooling/libs/lexer/tokenise.h>
#include <viua/tooling/libs/parser/parser.h>

std::string const OPTION_HELP_LONG = "--help";
std::string const OPTION_HELP_SHORT = "-h";
std::string const OPTION_VERSION = "--version";
std::string const OPTION_LEX = "--lex";

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
        std::cout << "        --lex         - perform just the lexical analysis, and return results as a"
                     "                        JSON list\n";

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
    bool lex_only = false;

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
        } else if (arg == OPTION_LEX) {
            parsed.lex_only = true;
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

using viua::tooling::libs::lexer::Token;
using token_index_type = std::vector<Token>::size_type;
static auto display_error_header(
    viua::tooling::errors::compile_time::Error const& error
    , std::string const& filename
) -> std::string {
    std::ostringstream o;

    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::COLOR_FG_WHITE;
    using viua::util::string::escape_sequences::COLOR_FG_CYAN;
    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::send_escape_seq;

    if (not error.str().empty()) {
        o << send_escape_seq(COLOR_FG_WHITE) << filename << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.line() + 1 << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.character() + 1 << send_escape_seq(ATTR_RESET);
        o << ": ";
        o << send_escape_seq(COLOR_FG_RED) << "error" << send_escape_seq(ATTR_RESET);
        o << ": " << error.what() << '\n';
    }
    for (auto const& note : error.notes()) {
        o << send_escape_seq(COLOR_FG_WHITE) << filename << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.line() + 1 << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.character() + 1 << send_escape_seq(ATTR_RESET);
        o << ": ";
        o << send_escape_seq(COLOR_FG_CYAN) << "note" << send_escape_seq(ATTR_RESET);
        o << ": " << note << '\n';
    }

    return o.str();
}
static auto underline_error_token(
    std::ostream& o
    , std::vector<Token> const& tokens
    , token_index_type i
    , viua::tooling::errors::compile_time::Error const& error
    , std::string::size_type const line_no_width
) -> void {
    /*
     * Indent is needed to align an aside note correctly.
     * The aside note is a additional piece of text that is attached to the
     * first underlined token, and servers as additional comment for the error
     * or note.
     */
    auto indent = std::ostringstream{};

    // message indent, ">>>>" on error lines
    o << "    ";
    indent << "    ";

    // to separate the ">>>>" on error lines from the line number
    o << ' ';
    indent << ' ';

    {
        // line number
        auto n = line_no_width;
        while (n--) {
            o << ' ';
            indent << ' ';
        }
    }

    /*
     * To separate line number from line content, but for comments we need
     * to preserve the indent without the pipe.
     */
    auto const comment_indent = indent.str()
        + ' '
        + viua::util::string::escape_sequences::COLOR_FG_GREEN_1
        + '>'
        + viua::util::string::escape_sequences::ATTR_RESET
        + ' ';
    o << " | ";
    indent << " | ";

    auto has_matched = false;
    auto has_matched_for_aside = false;
    auto underline = std::ostringstream{};

    while (i < tokens.size()) {
        auto const& each = tokens.at(i++);
        auto match = error.match(each);

        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::send_escape_seq;

        if (each == "\n") {
            o << send_escape_seq(ATTR_RESET);
            break;
        }

        auto c = char{match ? (has_matched ? '~' : '^') : ' '};
        has_matched = (has_matched or match);

        /*
         * Indentation for the aside should be increased as long as the token
         * the aside is attached to is not matched.
         */
        has_matched_for_aside = (has_matched_for_aside or error.match_aside(each));
        if (not has_matched_for_aside) {
            for (auto j = each.str().size(); j; --j) {
                indent << ' ';
            }
        }

        {
            auto n = each.str().size();

            /*
             * A singular decrement, and underlining character switch if neccessary.
             * Doing the check outside the loop to add significance to the fact that
             * only the first character of the underline is '^' and all subsequent
             * ones are '~'. We want underlining to look line this:
             *
             *      some tokens to underline
             *           ^~~~~~
             *
             * This 'if' below is just for the first '^' character.
             * The loop is for the string of '~'.
             */
            if (n--) {
                underline << c;
                if (match) {
                    c = '~';
                }
            }
            while (n--) {
                underline << c;
            }
        }
    }

    if (auto const u = underline.str(); not u.empty()) {
        using viua::util::string::escape_sequences::COLOR_FG_RED_1;
        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::send_escape_seq;

        o << send_escape_seq(COLOR_FG_RED_1);
        o << u;
        o << send_escape_seq(ATTR_RESET);
    }
    o << '\n';

    if (not error.aside().empty()) {
        using viua::util::string::escape_sequences::COLOR_FG_LIGHT_YELLOW;
        using viua::util::string::escape_sequences::COLOR_FG_LIGHT_GREEN;
        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::send_escape_seq;

        o << indent.str();
        o << send_escape_seq(COLOR_FG_LIGHT_YELLOW) << '^';
        o << send_escape_seq(COLOR_FG_LIGHT_GREEN) << ' ' << error.aside();
        o << send_escape_seq(ATTR_RESET);
        o << '\n';
    }

    if (not error.comments().empty()) {
        o << indent.str() << '\n';
        for (auto const& each : error.comments()) {
            o << comment_indent << each << '\n';
        }
        o << indent.str() << '\n';
    }
}
static auto display_error_line(
    std::ostream& o
    , std::vector<Token> const& tokens
    , viua::tooling::errors::compile_time::Error const& error
    , token_index_type i
    , std::string::size_type const line_no_width
) -> token_index_type {
    auto const token_line = error.line();

    using viua::util::string::escape_sequences::COLOR_FG_WHITE;
    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::COLOR_FG_YELLOW;
    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::send_escape_seq;

    o << send_escape_seq(COLOR_FG_RED);
    o << ">>>>";
    o << ' ';  // to separate the ">>>>" on error lines from the line number
    o << send_escape_seq(COLOR_FG_YELLOW);
    o << std::setw(static_cast<int>(line_no_width));
    o << token_line + 1;
    o << send_escape_seq(ATTR_RESET);
    o << " | ";     // to separate line number from line content
    o << send_escape_seq(COLOR_FG_WHITE);

    using viua::util::string::escape_sequences::COLOR_FG_ORANGE_RED_1;

    auto const original_i = i;

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        auto highlighted = false;
        if (error.match(tokens.at(i))) {
            o << send_escape_seq(COLOR_FG_ORANGE_RED_1);
            highlighted = true;
        }
        o << tokens.at(i++).str();
        if (highlighted) {
            o << send_escape_seq(COLOR_FG_WHITE);
        }
    }

    o << send_escape_seq(ATTR_RESET);

    underline_error_token(o, tokens, original_i, error, line_no_width);

    return i - 1;
}
static auto display_context_line(
    std::ostream& o
    , std::vector<Token> const& tokens
    , token_index_type i
    , std::string::size_type const line_no_width
) -> token_index_type {
    auto const token_line = tokens.at(i).line();

    o << "    ";  // message indent, ">>>>" on error lines
    o << ' ';     // to separate the ">>>>" on error lines from the line number
    o << std::setw(static_cast<int>(line_no_width));
    o << token_line + 1;
    o << " | ";     // to separate line number from line content

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        o << tokens.at(i++).str();
    }

    return i - 1;
}
static auto display_error_location(
    std::vector<Token> const& tokens
    , viua::tooling::errors::compile_time::Error const& error
) -> std::string {
    std::ostringstream o;

    auto const context_lines = size_t{2};
    auto const context_before = ((error.line() >= context_lines) ? (error.line() - context_lines) : 0);
    auto const context_after = (error.line() + context_lines);
    auto const line_no_width = std::to_string(context_after).size();

    auto lines_displayed = std::set<Token::Position_type>{};
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{0}; i < tokens.size(); ++i) {
        auto const& each = tokens.at(i);

        if (each.line() > context_after) {
            break;
        }
        if (each.line() < context_before) {
            continue;
        }
        if (lines_displayed.count(each.line())) {
            continue;
        }

        if (each.line() == error.line()) {
            i = display_error_line(o, tokens, error, i, line_no_width);
        } else {
            i = display_context_line(o, tokens, i, line_no_width);
        }

        lines_displayed.insert(each.line());
    }

    return o.str();
}
static auto display_error_in_context(
    std::ostream& o
    , std::vector<Token> const& tokens
    , viua::tooling::errors::compile_time::Error const& error
    , std::string const& filename
) -> void {
    o << display_error_header(error, filename);
    o << '\n';
    o << display_error_location(tokens, error);
}
static auto display_error_in_context(
    std::vector<Token> const& tokens
    , viua::tooling::errors::compile_time::Error_wrapper const& error
    , std::string const& filename
) -> void {
    std::ostringstream o;
    for (auto const& e : error.errors()) {
        display_error_in_context(o, tokens, e, filename);
        o << '\n';
    }
    std::cerr << o.str();
}

static auto to_json(viua::tooling::libs::lexer::Token const& token) -> std::string {
    auto o = std::ostringstream{};

    o << '{';
    o << std::quoted("line") << ':' << token.line();
    o << ',';
    o << std::quoted("character") << ':' << token.character();
    o << ',';
    o << std::quoted("content") << ':' << std::quoted(viua::util::string::ops::strencode(token.str()));
    o << ',';
    o << std::quoted("original") << ':' << std::quoted(viua::util::string::ops::strencode(token.original()));
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
    auto const tokens = [&raw_tokens, &parsed_args]() -> auto {
        try {
            return viua::tooling::libs::lexer::cook(raw_tokens);
        } catch (viua::tooling::errors::compile_time::Error_wrapper const& e) {
            display_error_in_context(raw_tokens, e, parsed_args.input_file);
            exit(1);
        }
    }();

    if (parsed_args.lex_only) {
        std::cerr << to_json(tokens) << std::endl;
        exit(0);
    }

    auto const cooked_fragments = [&raw_tokens, &tokens, &parsed_args]() -> auto {
        try {
            return viua::tooling::libs::parser::cook(
                parsed_args.input_file
                , viua::tooling::libs::parser::parse(tokens)
            );
        } catch (viua::tooling::errors::compile_time::Error_wrapper const& e) {
            display_error_in_context(raw_tokens, e, parsed_args.input_file);
            exit(1);
        }
    }();

    return 0;
}
