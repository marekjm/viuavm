/*
 *  Copyright (C) 2018, 2019 Marek Marecki
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
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <viua/runtime/imports.h>
#include <viua/tooling/errors/compile_time.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/lexer/tokenise.h>
#include <viua/tooling/libs/parser/parser.h>
#include <viua/tooling/libs/static_analyser/static_analyser.h>
#include <viua/util/filesystem.h>
#include <viua/util/string/escape_sequences.h>
#include <viua/util/string/ops.h>
#include <viua/loader.h>
#include <viua/version.h>

std::string const OPTION_HELP_LONG  = "--help";
std::string const OPTION_HELP_SHORT = "-h";
std::string const OPTION_VERSION    = "--version";
std::string const OPTION_LEX        = "--lex";

static auto usage(std::vector<std::string> const& args) -> bool {
    auto help_screen    = (args.size() == 1);
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
        std::cout << "Viua VM assembler version " << VERSION << '.' << MICRO
                  << std::endl;
    }
    if (help_screen) {
        if (version_screen) {
            std::cout << '\n';
        }
        std::cout << "SYNOPSIS\n";
        std::cout
            << "    " << args.at(0)
            << " [<option>...] [-o <output>] <source>.asm [<module>...]\n";
        std::cout << '\n';
        std::cout << "    <option> is any option that is accepted by the "
                     "assembler.\n";
        std::cout << "    <output> is the file to which the assembled bytecode "
                     "will be written.\n"
                     "     The default is to write output to `a.out` file.\n";
        std::cout << "    <source> is the name of the file containing the Viua "
                     "VM assembly language\n"
                     "    program to be assembled.\n";
        std::cout << "    <module> is a module that is to be statically linked "
                     "to the currently\n"
                     "    assembled file.\n";

        std::cout << '\n';

        std::cout << "OPTIONS\n";
        std::cout << "    -h, --help        - display help screen\n";
        std::cout << "        --version     - display version\n";
        std::cout << "    -o, --out <file>  - write output to <file>; the "
                     "default is to write to `a.out`\n";
        std::cout << "    -c, --lib         - assemble as a linkable module; "
                     "the default is to assemble\n"
                     "                        an executable\n";
        std::cout << "    -C, --verify      - perform static analysis, but do "
                     "not output bytecode\n";
        std::cout << "        --no-sa       - disable static analysis (useful "
                     "in case of false positives\n"
                     "                        being thrown by the SA engine)\n";
        std::cout << "        --lex         - perform just the lexical "
                     "analysis, and return results\n"
                     "                        as a JSON list\n";

        std::cout << '\n';

        std::cout << "COPYRIGHT\n";
        std::cout << "    Copyright (C) 2018-2019 Marek Marecki\n";
        std::cout << "    License GPLv3+: GNU GPL version 3 or later "
                     "<https://gnu.org/licenses/gpl.html>.\n";
        std::cout << "    This is free software: you are free to change and "
                     "redistribute it.\n";
        std::cout
            << "    There is NO WARRANTY, to the extent permitted by law.\n";
    }

    return (help_screen or version_screen);
}

static auto make_args(int const argc, char const* const argv[])
    -> std::vector<std::string> {
    auto args = std::vector<std::string>{};
    std::copy_n(argv, argc, std::back_inserter(args));
    return args;
}

struct Parsed_args {
    bool verbose         = false;
    bool enabled_sa      = true;
    bool sa_only         = false;
    bool linkable_module = false;
    bool lex_only        = false;

    std::string output_file = "a.out";
    std::string input_file;

    std::vector<std::string> linked_modules;

    std::map<std::string, std::string> options;
};

static auto parse_args(std::vector<std::string> const& args) -> Parsed_args {
    auto parsed = Parsed_args{};

    /*
     * Parse options at first. Stop parsing on first token that does not start
     * with
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
        } else if (arg == "--no-sa") {
            parsed.enabled_sa = false;
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
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unknown_option,
                arg);
        } else if (arg.find("--") != 0) {
            break;
        }
    }

    if (i == args.size()) {
        viua::tooling::errors::compile_time::display_error_and_exit(
            viua::tooling::errors::compile_time::Compile_time_error::
                No_input_file);
    }
    parsed.input_file = args.at(i++);
    if (parsed.input_file.size() == 0) {
        viua::tooling::errors::compile_time::display_error_and_exit(
            viua::tooling::errors::compile_time::Compile_time_error::
                No_input_file);
    }

    for (; i < args.size(); ++i) {
        parsed.linked_modules.push_back(args.at(i));
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
    viua::tooling::errors::compile_time::Error const& error,
    std::string const& filename) -> std::string {
    std::ostringstream o;

    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::COLOR_FG_CYAN;
    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::COLOR_FG_WHITE;
    using viua::util::string::escape_sequences::send_escape_seq;

    if (not error.empty()) {
        o << send_escape_seq(COLOR_FG_WHITE) << filename
          << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.line() + 1
          << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.character() + 1
          << send_escape_seq(ATTR_RESET);
        o << ": ";
        o << send_escape_seq(COLOR_FG_RED) << "error"
          << send_escape_seq(ATTR_RESET);
        o << ": " << error.what() << '\n';
    }
    for (auto const& note : error.notes()) {
        o << send_escape_seq(COLOR_FG_WHITE) << filename
          << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.line() + 1
          << send_escape_seq(ATTR_RESET);
        o << ':';
        o << send_escape_seq(COLOR_FG_WHITE) << error.character() + 1
          << send_escape_seq(ATTR_RESET);
        o << ": ";
        o << send_escape_seq(COLOR_FG_CYAN) << "note"
          << send_escape_seq(ATTR_RESET);
        o << ": " << note << '\n';
    }

    return o.str();
}
static auto underline_error_token(
    std::ostream& o,
    std::vector<Token> const& tokens,
    token_index_type i,
    viua::tooling::errors::compile_time::Error const& error,
    std::string::size_type const line_no_width) -> void {
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
    auto const comment_indent =
        indent.str() + ' '
        + viua::util::string::escape_sequences::COLOR_FG_GREEN_1 + '>'
        + viua::util::string::escape_sequences::ATTR_RESET + ' ';
    o << " | ";
    indent << " | ";

    auto has_matched           = false;
    auto has_matched_for_aside = false;
    auto underline             = std::ostringstream{};

    while (i < tokens.size()) {
        auto const& each = tokens.at(i++);
        auto match       = error.match(each);

        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::send_escape_seq;

        if (each == "\n") {
            o << send_escape_seq(ATTR_RESET);
            break;
        }

        auto c      = char{match ? (has_matched ? '~' : '^') : ' '};
        has_matched = (has_matched or match);

        /*
         * Indentation for the aside should be increased as long as the token
         * the aside is attached to is not matched.
         */
        has_matched_for_aside =
            (has_matched_for_aside or error.match_aside(each));
        if (not has_matched_for_aside) {
            for (auto j = each.str().size(); j; --j) {
                indent << ' ';
            }
        }

        {
            auto n = each.str().size();

            /*
             * A singular decrement, and underlining character switch if
             * neccessary. Doing the check outside the loop to add significance
             * to the fact that only the first character of the underline is '^'
             * and all subsequent ones are '~'. We want underlining to look line
             * this:
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
        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::COLOR_FG_RED_1;
        using viua::util::string::escape_sequences::send_escape_seq;

        o << send_escape_seq(COLOR_FG_RED_1);
        o << u;
        o << send_escape_seq(ATTR_RESET);
    }
    o << '\n';

    if (not error.aside().empty()) {
        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::COLOR_FG_LIGHT_GREEN;
        using viua::util::string::escape_sequences::COLOR_FG_LIGHT_YELLOW;
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
    std::ostream& o,
    std::vector<Token> const& tokens,
    viua::tooling::errors::compile_time::Error const& error,
    token_index_type i,
    std::string::size_type const line_no_width) -> token_index_type {
    auto const token_line = error.line();

    using viua::util::string::escape_sequences::ATTR_RESET;
    using viua::util::string::escape_sequences::COLOR_FG_RED;
    using viua::util::string::escape_sequences::COLOR_FG_WHITE;
    using viua::util::string::escape_sequences::COLOR_FG_YELLOW;
    using viua::util::string::escape_sequences::send_escape_seq;

    o << send_escape_seq(COLOR_FG_RED);
    o << ">>>>";
    o << ' ';  // to separate the ">>>>" on error lines from the line number
    o << send_escape_seq(COLOR_FG_YELLOW);
    o << std::setw(static_cast<int>(line_no_width));
    o << token_line + 1;
    o << send_escape_seq(ATTR_RESET);
    o << " | ";  // to separate line number from line content
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
static auto display_context_line(std::ostream& o,
                                 std::vector<Token> const& tokens,
                                 token_index_type i,
                                 std::string::size_type const line_no_width)
    -> token_index_type {
    auto const token_line = tokens.at(i).line();

    o << "    ";  // message indent, ">>>>" on error lines
    o << ' ';     // to separate the ">>>>" on error lines from the line number
    o << std::setw(static_cast<int>(line_no_width));
    o << token_line + 1;
    o << " | ";  // to separate line number from line content

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        o << tokens.at(i++).str();
    }

    return i - 1;
}
static auto display_error_location(
    std::vector<Token> const& tokens,
    viua::tooling::errors::compile_time::Error const& error) -> std::string {
    std::ostringstream o;

    auto const context_lines = size_t{2};
    auto const context_before =
        ((error.line() >= context_lines) ? (error.line() - context_lines) : 0);
    auto const context_after = (error.line() + context_lines);
    auto const line_no_width = std::to_string(context_after + 1).size();

    auto lines_displayed = std::set<Token::Position_type>{};
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};
         i < tokens.size();
         ++i) {
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
    std::ostream& o,
    std::vector<Token> const& tokens,
    viua::tooling::errors::compile_time::Error const& error,
    std::string const& filename) -> void {
    o << display_error_header(error, filename);
    o << '\n';
    o << display_error_location(tokens, error);
}
static auto display_error_in_context(
    std::vector<Token> const& tokens,
    viua::tooling::errors::compile_time::Error_wrapper const& error,
    std::string const& filename) -> void {
    std::ostringstream o;
    for (auto const& e : error.errors()) {
        display_error_in_context(o, tokens, e, filename);
        o << '\n';
    }
    std::cerr << o.str();
}

static auto to_json(viua::tooling::libs::lexer::Token const& token)
    -> std::string {
    auto o = std::ostringstream{};

    o << '{';
    o << std::quoted("line") << ':' << token.line();
    o << ',';
    o << std::quoted("character") << ':' << token.character();
    o << ',';
    o << std::quoted("content") << ':'
      << std::quoted(viua::util::string::ops::strencode(token.str()));
    o << ',';
    o << std::quoted("original") << ':'
      << std::quoted(viua::util::string::ops::strencode(token.original()));
    o << '}';

    return o.str();
}
static auto to_json(
    std::vector<viua::tooling::libs::lexer::Token> const& tokens)
    -> std::string {
    auto o = std::ostringstream{};

    o << '[' << '\n';
    o << ' ' << to_json(tokens.at(0)) << '\n';
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};
         i < tokens.size();
         ++i) {
        o << ',' << to_json(tokens.at(i)) << '\n';
    }
    o << ']';

    return o.str();
}

struct Symbols {
    enum class Kind {
        Function,
        Closure,
        Block,
    };
    enum class Linkage_location {
        Local,      /* symbols defined in current compilation unit */
        External,   /* symbols defined in other compilation units */
    };
    struct Symbol {
        Kind const kind;
        Linkage_location const linkage_location;
        std::string const source_file;
        std::string const source_module;
        std::string const name;

        Symbol(Kind const k, Linkage_location const ll, std::string sf
                , std::string sm, std::string n)
            : kind{k}
            , linkage_location{ll}
            , source_file{std::move(sf)}
            , source_module{std::move(sm)}
            , name{std::move(n)}
        {}
    };

    std::map<std::string, Symbol> available;
};

static auto to_string(Symbols::Kind const k) -> std::string {
    switch (k) {
        case Symbols::Kind::Function:
            return "function";
        case Symbols::Kind::Closure:
            return "closure";
        case Symbols::Kind::Block:
            return "block";
        default:
            throw std::out_of_range{"invalid symbol kind"};
    }
}

static auto to_string(Symbols::Linkage_location const ll) -> std::string {
    switch (ll) {
        case Symbols::Linkage_location::Local:
            return "local";
        case Symbols::Linkage_location::External:
            return "external";
        default:
            throw std::out_of_range{"invalid linkage location"};
    }
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
            viua::tooling::errors::compile_time::Compile_time_error::Not_a_file,
            parsed_args.input_file);
    }

    auto const source     = read_file(parsed_args.input_file);
    auto const raw_tokens = viua::tooling::libs::lexer::tokenise(source);
    auto const tokens     = [&raw_tokens, &parsed_args ]() -> auto {
        try {
            return viua::tooling::libs::lexer::cook(raw_tokens);
        } catch (viua::tooling::errors::compile_time::Error_wrapper const& e) {
            display_error_in_context(raw_tokens, e, parsed_args.input_file);
            exit(1);
        }
    }
    ();

    if (parsed_args.lex_only) {
        std::cerr << to_json(tokens) << std::endl;
        exit(0);
    }

    auto const cooked_fragments =
        [&raw_tokens, &tokens, &parsed_args ]() -> auto {
        try {
            return viua::tooling::libs::parser::cook(
                parsed_args.input_file,
                viua::tooling::libs::parser::parse(tokens));
        } catch (viua::tooling::errors::compile_time::Error_wrapper const& e) {
            display_error_in_context(raw_tokens, e, parsed_args.input_file);
            exit(1);
        }
    }
    ();

    for (auto const& each : cooked_fragments.info_fragments) {
        std::cout << "information:     " << each->key << " = " << each->value
                  << std::endl;
    }
    for (auto const& each : cooked_fragments.extern_function_fragments) {
        std::cout << "extern function: " << each->function_name << '/'
                  << each->arity << std::endl;
    }
    for (auto const& each : cooked_fragments.extern_block_fragments) {
        std::cout << "extern block:    " << each->block_name << std::endl;
    }
    for (auto const& each : cooked_fragments.import_fragments) {
        std::cout << "imported module: " << each->module_name;
        if (not each->attributes.empty()) {
            std::cout << " (";
            for (auto const& attr : each->attributes) {
                std::cout << ' ' << attr;
            }
            std::cout << " )";
        }
        std::cout << std::endl;
    }
    for (auto const& each : cooked_fragments.function_fragments) {
        auto const& fn =
            *static_cast<viua::tooling::libs::parser::Function_head const*>(
                each.second.lines.at(0).get());
        std::cout << "function:        " << fn.function_name << '/' << fn.arity;
        if (not fn.attributes.empty()) {
            std::cout << " [[";
            for (auto const& attr : fn.attributes) {
                std::cout << ' ' << attr;
            }
            std::cout << " ]]";
        }
        std::cout << std::endl;
    }
    for (auto const& each : cooked_fragments.closure_fragments) {
        auto const& fn =
            *static_cast<viua::tooling::libs::parser::Closure_head const*>(
                each.second.lines.at(0).get());
        std::cout << "closure:         " << fn.function_name << '/' << fn.arity;
        if (not fn.attributes.empty()) {
            std::cout << " [[";
            for (auto const& attr : fn.attributes) {
                std::cout << ' ' << attr;
            }
            std::cout << " ]]";
        }
        std::cout << std::endl;
    }
    for (auto const& each : cooked_fragments.block_fragments) {
        auto const& bl =
            *static_cast<viua::tooling::libs::parser::Block_head const*>(
                each.second.lines.at(0).get());
        std::cout << "block:           " << bl.name;
        if (not bl.attributes.empty()) {
            std::cout << " [[";
            for (auto const& attr : bl.attributes) {
                std::cout << ' ' << attr;
            }
            std::cout << " ]]";
        }
        std::cout << std::endl;
    }

    if (parsed_args.enabled_sa) {
        try {
            viua::tooling::libs::static_analyser::analyse(cooked_fragments);
        } catch (viua::tooling::errors::compile_time::Error_wrapper const& e) {
            display_error_in_context(raw_tokens, e, parsed_args.input_file);
            exit(1);
        }
    }

    if (parsed_args.sa_only) {
        return 0;
    }

    auto symbols = Symbols{};
    for (auto const& each : cooked_fragments.function_fragments) {
        auto const& fn =
            *static_cast<viua::tooling::libs::parser::Function_head const*>(
                each.second.lines.at(0).get());
        auto const name = fn.function_name + "/" + std::to_string(fn.arity);
        symbols.available.insert({ name, Symbols::Symbol{
            Symbols::Kind::Function,
            Symbols::Linkage_location::Local,
            parsed_args.input_file,
            "<this>",
            name
        }});
    }

    {
        using viua::util::string::escape_sequences::ATTR_RESET;
        using viua::util::string::escape_sequences::COLOR_FG_RED;
        using viua::util::string::escape_sequences::COLOR_FG_WHITE;
        using viua::util::string::escape_sequences::send_escape_seq;

        for (auto const& each : parsed_args.linked_modules) {
            if (viua::util::filesystem::is_file(each)) {
                continue;
            }

            std::cerr << send_escape_seq(COLOR_FG_WHITE) << parsed_args.input_file
                      << send_escape_seq(ATTR_RESET) << ':'
                      << send_escape_seq(COLOR_FG_RED) << "error"
                      << send_escape_seq(ATTR_RESET)
                      << ": file cannot be linked: not found: \""
                      << send_escape_seq(COLOR_FG_WHITE) << each
                      << send_escape_seq(ATTR_RESET) << "\"\n";
            return 1;
        }

        auto static_imports = std::vector<std::string>{};
        auto dynamic_imports = std::vector<std::pair<std::string, std::string>>{};

        std::copy(
              parsed_args.linked_modules.begin()
            , parsed_args.linked_modules.end()
            , std::back_inserter(static_imports)
        );

        auto const STATIC_IMPORT_TAG  = std::string{"static"};
        auto const DYNAMIC_IMPORT_TAG = std::string{"dynamic"};

        for (auto const& each : cooked_fragments.import_fragments) {
            auto const& name = each->module_name;
            auto const& attrs = each->attributes;

            using viua::util::string::escape_sequences::ATTR_RESET;
            using viua::util::string::escape_sequences::COLOR_FG_CYAN;
            using viua::util::string::escape_sequences::COLOR_FG_RED;
            using viua::util::string::escape_sequences::COLOR_FG_WHITE;
            using viua::util::string::escape_sequences::send_escape_seq;

            auto const module_path = viua::runtime::imports::find_module(name);
            if (not module_path.has_value()) {
                std::cerr << send_escape_seq(COLOR_FG_WHITE) << parsed_args.input_file
                          << send_escape_seq(ATTR_RESET) << ':'
                          << send_escape_seq(COLOR_FG_RED) << "error"
                          << send_escape_seq(ATTR_RESET)
                          << ": did not find module \""
                          << send_escape_seq(COLOR_FG_WHITE) << name
                          << send_escape_seq(ATTR_RESET) << "\"\n";
                return 1;
            }

            using viua::runtime::imports::Module_type;
            if (attrs.count(STATIC_IMPORT_TAG)
                and module_path->first != Module_type::Bytecode) {
                std::cerr
                    << send_escape_seq(COLOR_FG_WHITE) << parsed_args.input_file
                    << send_escape_seq(ATTR_RESET) << ':'
                    << send_escape_seq(COLOR_FG_RED) << "error"
                    << send_escape_seq(ATTR_RESET)
                    << ": only bytecode modules may be statically imported: \""
                    << send_escape_seq(COLOR_FG_WHITE) << name
                    << send_escape_seq(ATTR_RESET) << "\" is found in "
                    << module_path->second
                    << "\n";
                return 1;
            }

            if (attrs.count(STATIC_IMPORT_TAG)) {
                static_imports.push_back(module_path->second);
            } else if (attrs.count(DYNAMIC_IMPORT_TAG)) {
                dynamic_imports.push_back({name, module_path->second});
            } else {
                std::cerr << send_escape_seq(COLOR_FG_WHITE) << parsed_args.input_file
                          << send_escape_seq(ATTR_RESET) << ':'
                          << send_escape_seq(COLOR_FG_RED) << "error"
                          << send_escape_seq(ATTR_RESET)
                          << ": link mode not specified for module \""
                          << send_escape_seq(COLOR_FG_WHITE) << name
                          << send_escape_seq(ATTR_RESET) << "\"\n";
                std::cerr << send_escape_seq(COLOR_FG_WHITE) << parsed_args.input_file
                          << send_escape_seq(ATTR_RESET) << ':'
                          << send_escape_seq(COLOR_FG_CYAN) << "note"
                          << send_escape_seq(ATTR_RESET)
                          << ": expected either \""
                          << send_escape_seq(COLOR_FG_WHITE) << "static"
                          << send_escape_seq(ATTR_RESET) << '"' << " or \""
                          << send_escape_seq(COLOR_FG_WHITE) << "dynamic"
                          << send_escape_seq(ATTR_RESET) << "\"\n";
                return 1;
            }
        }

        std::cerr << "static imports:\n";
        for (auto const& each : static_imports) {
            std::cerr << "    " << each << "\n";
        }

        {
            for (auto const& each : static_imports) {
                auto loader = Loader{each};
                loader.load();

                {
                    auto fns = loader.get_functions();
                    if (fns.empty()) {
                        std::cerr << send_escape_seq(COLOR_FG_WHITE) << parsed_args.input_file
                                  << send_escape_seq(ATTR_RESET) << ": "
                                  << send_escape_seq(COLOR_FG_RED) << "warning"
                                  << send_escape_seq(ATTR_RESET) << ": "
                                  << "static-linked module \""
                                  << send_escape_seq(COLOR_FG_WHITE) << each
                                  << send_escape_seq(ATTR_RESET)
                                  << "\" defines no functions\n";
                    }

                    for (auto const& fn : fns) {
                        if (symbols.available.count(fn)) {
                            std::cerr
                                << send_escape_seq(COLOR_FG_WHITE)
                                << parsed_args.input_file
                                << send_escape_seq(ATTR_RESET) << ": "
                                << send_escape_seq(COLOR_FG_RED) << "error"
                                << send_escape_seq(ATTR_RESET) << ": "
                                << "duplicate symbol '"
                                << fn
                                << "' found when importing '"
                                << each
                                << "', previously found in '"
                                << symbols.available.at(fn).source_file
                                << "'"
                                << "\n";
                            return 1;
                        }
                    }

                    for (auto const& fn : fns) {
                        symbols.available.insert({ fn, Symbols::Symbol{
                            Symbols::Kind::Function,
                            Symbols::Linkage_location::External,
                            each,
                            each,   // FIXME specify module name, not file path
                            name
                        }});
                    }
                }
            }
        }
    }

    std::cout << "available symbols:\n";
    for (auto const& [ name, each ] : symbols.available) {
        std::cout << "    " << name << ": ";
        std::cout << to_string(each.linkage_location) << " " << to_string(each.kind) << "\n";
        std::cout << "        from module: " << each.source_module << " (" << each.source_file << ")\n";
    }

    return 0;
}
