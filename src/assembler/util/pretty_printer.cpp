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

#include <unistd.h>

#include <iomanip>
#include <iostream>

#include <viua/assembler/util/pretty_printer.h>
#include <viua/front/asm.h>
#include <viua/support/string.h>


auto viua::assembler::util::pretty_printer::send_control_seq(
    std::string const& mode) -> std::string
{
    static auto is_terminal = isatty(1);
    static std::string env_color_flag{
        getenv("VIUAVM_ASM_COLOUR") ? getenv("VIUAVM_ASM_COLOUR") : "default"};

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


auto viua::assembler::util::pretty_printer::underline_error_token(
    std::vector<viua::cg::lex::Token> const& tokens,
    decltype(tokens.size()) i,
    viua::cg::lex::Invalid_syntax const& error) -> void
{
    /*
     * Indent is needed to align an aside note correctly.
     * The aside note is a additional piece of text that is attached to the
     * first underlined token, and servers as additional comment for the error
     * or note.
     */
    std::ostringstream indent;

    /*
     * Remember to increase the indent also.
     */
    std::cout << "     ";
    indent << "     ";

    /*
     * Account for width of the stringified line number when indenting error
     * lines.
     */
    auto len = str::stringify((error.line() + 1), false).size();
    while (len--) {
        std::cout << ' ';
        indent << ' ';
    }

    /*
     * Insert additional space between line number and source code listing to
     * aid readability.
     */
    std::cout << ' ';
    indent << ' ';

    bool has_matched           = false;
    bool has_matched_for_aside = false;
    while (i < tokens.size()) {
        auto const& each = tokens.at(i++);
        bool match       = error.match(each);

        if (each == "\n") {
            std::cout << send_control_seq(ATTR_RESET);
            break;
        }

        if (match) {
            std::cout << send_control_seq(COLOR_FG_RED_1);
        }

        char c = (match ? (has_matched ? '~' : '^') : ' ');

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

        len = each.str().size();

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
        if (len--) {
            std::cout << c;
            if (match) {
                c = '~';
            }
        }
        while (len--) {
            std::cout << c;
        }

        if (match) {
            std::cout << send_control_seq(ATTR_RESET);
        }
    }

    std::cout << '\n';

    if (error.aside().size()) {
        std::cout << indent.str();
        std::cout << send_control_seq(COLOR_FG_RED_1) << '^'
                  << send_control_seq(COLOR_FG_LIGHT_GREEN) << ' '
                  << error.aside() << send_control_seq(ATTR_RESET) << std::endl;
    }
}
auto viua::assembler::util::pretty_printer::display_error_line(
    std::vector<viua::cg::lex::Token> const& tokens,
    viua::cg::lex::Invalid_syntax const& error,
    decltype(tokens.size()) i,
    size_t const line_no_width) -> decltype(i)
{
    const auto token_line = tokens.at(i).line();

    std::cout << send_control_seq(COLOR_FG_RED);
    std::cout << ">>>>";  // message indent, ">>>>" on error line
    std::cout << ' ';

    std::cout << send_control_seq(COLOR_FG_YELLOW);
    std::cout << std::setw(static_cast<int>(line_no_width));
    std::cout << token_line + 1;
    std::cout << ' ';

    auto original_i = i;

    std::cout << send_control_seq(COLOR_FG_WHITE);
    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        bool highlighted = false;
        if (error.match(tokens.at(i))) {
            std::cout << send_control_seq(COLOR_FG_ORANGE_RED_1);
            highlighted = true;
        }
        std::cout << tokens.at(i++).str();
        if (highlighted) {
            std::cout << send_control_seq(COLOR_FG_WHITE);
        }
    }

    std::cout << send_control_seq(ATTR_RESET);

    underline_error_token(tokens, original_i, error);

    return i;
}
auto viua::assembler::util::pretty_printer::display_context_line(
    std::vector<viua::cg::lex::Token> const& tokens,
    viua::cg::lex::Invalid_syntax const&,
    decltype(tokens.size()) i,
    size_t const line_no_width) -> decltype(i)
{
    const auto token_line = tokens.at(i).line();

    std::cout << "    ";  // message indent, ">>>>" on error line
    std::cout << ' ';
    std::cout << std::setw(static_cast<int>(line_no_width));
    std::cout << token_line + 1;
    std::cout << ' ';

    while (i < tokens.size() and tokens.at(i).line() == token_line) {
        std::cout << tokens.at(i++).str();
    }

    return i;
}
auto viua::assembler::util::pretty_printer::display_error_header(
    viua::cg::lex::Invalid_syntax const& error,
    std::string const& filename) -> void
{
    if (error.str().size()) {
        std::cout << send_control_seq(COLOR_FG_WHITE) << filename << ':'
                  << error.line() + 1 << ':' << error.character() + 1 << ':'
                  << send_control_seq(ATTR_RESET) << ' ';
        std::cout << send_control_seq(COLOR_FG_RED) << "error"
                  << send_control_seq(ATTR_RESET) << ": " << error.what()
                  << std::endl;
    }
    if (error.notes().size()) {
        for (auto const& note : error.notes()) {
            std::cout << send_control_seq(COLOR_FG_WHITE) << filename << ':'
                      << error.line() + 1 << ':' << error.character() + 1 << ':'
                      << send_control_seq(ATTR_RESET) << ' ';
            std::cout << send_control_seq(COLOR_FG_CYAN) << "note"
                      << send_control_seq(ATTR_RESET) << ": " << note
                      << std::endl;
        }
    }
}
auto viua::assembler::util::pretty_printer::display_error_location(
    std::vector<viua::cg::lex::Token> const& tokens,
    const viua::cg::lex::Invalid_syntax error) -> void
{
    const unsigned context_lines          = 2;
    decltype(error.line()) context_before = 0,
                           context_after  = (error.line() + context_lines);
    if (error.line() >= context_lines) {
        context_before = (error.line() - context_lines);
    }

    const auto line_no_width = std::to_string(context_after).size();

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0;
         i < tokens.size();) {
        if (tokens.at(i).line() > context_after) {
            break;
        }
        if (tokens.at(i).line() >= context_before) {
            if (tokens.at(i).line() == error.line()) {
                i = display_error_line(tokens, error, i, line_no_width);
            } else {
                i = display_context_line(tokens, error, i, line_no_width);
            }
            continue;
        }
        ++i;
    }
}
auto viua::assembler::util::pretty_printer::display_error_in_context(
    std::vector<viua::cg::lex::Token> const& tokens,
    const viua::cg::lex::Invalid_syntax error,
    std::string const& filename) -> void
{
    display_error_header(error, filename);
    std::cout << "\n";
    display_error_location(tokens, error);
}
auto viua::assembler::util::pretty_printer::display_error_in_context(
    std::vector<viua::cg::lex::Token> const& tokens,
    const viua::cg::lex::Traced_syntax_error error,
    std::string const& filename) -> void
{
    for (auto const& e : error.errors) {
        display_error_in_context(tokens, e, filename);
        std::cout << "\n";
    }
}
