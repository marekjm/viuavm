/*
 *  Copyright (C) 2023 Marek Marecki
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

#include <elf.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>
#include <string_view>

#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>

using viua::support::string::quote_fancy;

namespace viua::libs::stage {
auto view_line_of(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    /*
     * We want to get a view of a single line out of a view of the entire source
     * code. This task can be achieved by finding the newlines before and after
     * the offset at which out location is located, and cutting the portion of
     * the string inside these two bounds.
     */
    {
        /*
         * The ending newline is easy - just find the next occurence after the
         * location we are interested in. If no can be found then we have
         * nothing to do because the line we want to view is the last line of
         * the source code.
         */
        auto line_end = size_t{0};
        line_end      = sv.find('\n', loc.offset);

        if (line_end != std::string::npos) {
            sv.remove_suffix(sv.size() - line_end);
        }
    }
    {
        /*
         * The starting newline is tricky. We need to find it, but if we do not
         * then it means that the line we want to view is the very fist line of
         * the source code...
         */
        auto line_begin = size_t{0};
        line_begin      = sv.rfind('\n', (loc.offset ? (loc.offset - 1) : 0));

        /*
         * ...and we have to treat it a little bit different. For any other line
         * we increase the beginning offset by 1 (to skip the actual newline
         * character), and for the first one we substitute the npos for 0 (to
         * start from the beginning).
         */
        line_begin = (line_begin == std::string::npos) ? 0 : (line_begin + 1);

        sv.remove_prefix(line_begin);
    }

    return sv;
}

auto view_line_before(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    auto line_end   = size_t{0};
    auto line_begin = size_t{0};

    line_end = sv.rfind('\n', (loc.offset ? (loc.offset - 1) : 0));
    if (line_end != std::string::npos) {
        sv.remove_suffix(sv.size() - line_end);
    }

    line_begin = sv.rfind('\n');
    if (line_begin != std::string::npos) {
        sv.remove_prefix(line_begin + 1);
    }

    return sv;
}

auto view_line_after(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    auto line_end   = size_t{0};
    auto line_begin = size_t{0};

    line_begin = sv.find('\n', loc.offset);
    if (line_begin != std::string::npos) {
        sv.remove_prefix(line_begin + 1);
    }

    line_end = sv.find('\n');
    if (line_end != std::string::npos) {
        sv.remove_suffix(sv.size() - line_end);
    }

    return sv;
}

auto cook_spans(
    std::vector<viua::libs::errors::compile_time::Error::span_type> raw)
    -> std::vector<std::tuple<bool, size_t, size_t, bool>>
{
    /*
     * In case of synthesised lexemes, there may be several of them that have
     * the same offset. Naively including all of them in spans produces... weird
     * results when the output is formatted for an error report.
     *
     * Let's only use the first span at a given offsets.
     */
    auto seen_offsets = std::set<size_t>{};

    /*
     * Cooked spans vector covers a whole line, so we need to record the
     * following information:
     *
     *   1/ whether a region should be highlighted
     *   2/ where does a region start
     *   3/ how long it is
     *
     * This makes life easy for us later, when we can just std::string::substr()
     * to success, and switch on the highlight-or-not member of the tuple.
     */
    auto cooked = std::vector<std::tuple<bool, size_t, size_t, bool>>{};
    if (raw.empty()) {
        return cooked;
    }

    /*
     * Cooked spans cover the whole line, both regions which should be
     * highlighted and those which should not be. This means that whitespace
     * must also be considered.
     *
     * Thus, if the first span is not at offset 0 we must produce a span that
     * covers the region of the line leading to that first span (eg, the
     * indentation).
     *
     * Otherwise, we have to push the first raw span. The cooking loop needs
     * to look at the last cooked span when cooking a raw one, so the cooked
     * vector MUST contain AT LEAST ONE cooked span.
     */
    if (std::get<0>(raw.front()) != 0) {
        cooked.emplace_back(false, 0, std::get<0>(raw.front()), false);
        seen_offsets.insert(0);
    } else {
        auto const& each                         = raw.front();
        auto const [character, offset, advisory] = each;
        cooked.emplace_back(true, character, offset, advisory);
        seen_offsets.insert(character);
    }

    for (auto const& each : raw) {
        /*
         * Check if we have a gap between last cooked span and the next raw one.
         * In the final cooked span vector the MUST BE NO GAPS so we have to
         * fill any that are found. Of course, gaps should not be highlighted.
         */
        {
            auto const& [ignored_hl, offset, size, ignored_advisory] =
                cooked.back();
            if (auto const want = (offset + size); want < std::get<0>(each)) {
                if (not seen_offsets.contains(want)) {
                    cooked.emplace_back(
                        false, want, (std::get<0>(each) - want), false);
                    seen_offsets.insert(want);
                }
            }
        }

        /*
         * Last, but not least, cook the actual raw span.
         */
        auto const& [offset, size, advisory] = each;
        if (not seen_offsets.contains(offset)) {
            cooked.emplace_back(true, offset, size, advisory);
            seen_offsets.insert(offset);
        }
    }

    return cooked;
}

auto display_error(std::filesystem::path source_path,
                   std::string_view source_text,
                   size_t const line_no_width,
                   viua::libs::errors::compile_time::Error const& e) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    constexpr auto SEPARATOR_SOURCE = std::string_view{" | "};
    constexpr auto SEPARATOR_ASIDE  = std::string_view{" . "};
    constexpr auto ERROR_MARKER     = std::string_view{" => "};
    constexpr auto BOX_DRAWINGS_BLACK_RIGHT_POINTING_TRIANGLE
        [[maybe_unused]]             = std::string_view{"▶"};  // u+25b6
    constexpr auto ERROR_MARKER_SIZE = ERROR_MARKER.size();

    if (auto const msg = e.str(); not msg.empty()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (e.line() + 1) << esc(2, ATTR_RESET) << ':'
                  << esc(2, COLOR_FG_WHITE) << (e.character() + 1)
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET) << ": " << e.str() << "\n";
    }
    for (auto const& each : e.notes()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (e.line() + 1) << esc(2, ATTR_RESET) << ':'
                  << esc(2, COLOR_FG_WHITE) << (e.character() + 1)
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                  << "note" << esc(2, ATTR_RESET) << ": ";
        if (each.find('\n') == std::string::npos) {
            std::cerr << each << "\n";
        } else {
            auto const prefix_length =
                source_path.string().size()
                + std::to_string(e.line() + 1).size()
                + std::to_string(e.character() + 1).size() + 6  // for ": note"
                + 2  // for ":" after source path and line number
                ;

            auto sv = std::string_view{each};
            std::cerr << sv.substr(0, sv.find('\n')) << '\n';

            do {
                sv.remove_prefix(sv.find('\n') + 1);
                std::cerr << std::string(prefix_length, ' ') << "| "
                          << sv.substr(0, sv.find('\n')) << '\n';
            } while (sv.find('\n') != std::string::npos);
        }
    }

    /*
     * The separator to put some space between the command and the error
     * report.
     */
    std::cerr << std::string(ERROR_MARKER_SIZE, ' ')
              << std::string(line_no_width, ' ') << SEPARATOR_SOURCE << "\n";

    if (e.line()) {
        std::cerr << std::string(ERROR_MARKER_SIZE, ' ')
                  << std::setw(line_no_width) << e.line() << SEPARATOR_SOURCE
                  << view_line_before(source_text, e.location()) << "\n";
    }

    auto source_line    = std::ostringstream{};
    auto highlight_line = std::ostringstream{};

    if constexpr (false) {
        auto line = view_line_of(source_text, e.location());

        source_line << esc(2, COLOR_FG_RED) << ERROR_MARKER
                    << std::setw(line_no_width) << (e.line() + 1)
                    << esc(2, COLOR_FG_WHITE) << SEPARATOR_SOURCE;
        highlight_line << std::string(ERROR_MARKER_SIZE, ' ')
                       << std::string(line_no_width, ' ') << SEPARATOR_SOURCE;

        source_line << std::string_view{line.data(), e.character()};
        highlight_line << std::string(e.character(), ' ');
        line.remove_prefix(e.character());

        /*
         * This if is required because of TERMINATOR tokens in unexpected
         * places. In case a TERMINATOR token is the cause of the error it
         * will not appear in line. If we attempted to shift line's head, it
         * would be removing a prefix from an empty std::string_view which
         * is undefined behaviour.
         *
         * I think the "bad TERMINATOR" is the only situation when this is
         * important.
         *
         * When it happens, we just don't print the terminator (which is a
         * newline), because a newline character will be added anyway.
         */
        if (not line.empty()) {
            source_line << esc(2, COLOR_FG_RED_1) << e.main().text
                        << esc(2, ATTR_RESET);
            line.remove_prefix(e.main().text.size());
        }
        highlight_line << esc(2, COLOR_FG_RED) << '^';
        highlight_line << esc(2, COLOR_FG_ORANGE_RED_1)
                       << std::string((e.main().text.size() - 1), '~')
                       << esc(2, ATTR_RESET);

        source_line << esc(2, COLOR_FG_WHITE) << line << esc(2, ATTR_RESET);
    }

    {
        auto line = view_line_of(source_text, e.location());

        source_line << esc(2, COLOR_FG_RED) << ERROR_MARKER
                    << std::setw(line_no_width) << (e.line() + 1)
                    << esc(2, COLOR_FG_WHITE) << SEPARATOR_SOURCE;
        highlight_line << std::string(ERROR_MARKER_SIZE, ' ')
                       << std::string(line_no_width, ' ') << SEPARATOR_SOURCE;

        auto const spans = cook_spans(e.spans());
        for (auto const& each : spans) {
            auto const& [hl, offset, size, advisory] = each;

            /*
             * The error is composed of one MAIN lexeme, and zero or more EXTRA
             * lexemes. The token that caused the error may have been composed
             * of more than one lexeme; or the error may be a combination of
             * several tokens.
             *
             * In both cases MAIN is the most important part, and EXTRA are the
             * circumstances which contributed to the error.
             */
            constexpr auto COLOR_MAIN  = COLOR_FG_RED_1;
            constexpr auto COLOR_EXTRA = COLOR_FG_RED;

            /*
             * ADVISORY highlights are used to call attention to other parts of
             * the line, which did not directly contributed to the error, but
             * which are important anyway.
             *
             * For example: function "foo" is declared as both "local" and the
             * entry point of the program. This is an error.
             *
             * The "local" tag is the MAIN token, because it is the direct cause
             * of the error.
             *
             * The "entry_point" tag is an EXTRA token, because it contributed
             * to the error happening - without it the "local" tag would be OK.
             *
             * The name of the function is an ADVISORY token. It does not really
             * matter *what* the value of the token is, just that it is this
             * particular one.
             */
            constexpr auto COLOR_ADVISORY = std::string_view{"\x1b[38;5;26m"};

            if (hl and offset == e.character()) {
                source_line << esc(2, COLOR_MAIN);
                highlight_line << esc(2, COLOR_MAIN) << '^'
                               << esc(2, COLOR_EXTRA)
                               << std::string(size - 1, '~');
            } else if (hl and not advisory) {
                source_line << esc(2, COLOR_EXTRA);
                highlight_line << esc(2, COLOR_EXTRA) << std::string(size, '~');
            } else if (hl and advisory) {
                source_line << esc(2, COLOR_ADVISORY);
                highlight_line << esc(2, COLOR_EXTRA) << std::string(size, '~');
            } else {
                source_line << esc(2, COLOR_FG_WHITE);
                highlight_line << std::string(size, ' ');
            }
            source_line << line.substr(offset, size);
        }
        auto const final_offset =
            std::get<1>(spans.back()) + std::get<2>(spans.back());
        if (final_offset < line.size()) {
            source_line << esc(2, COLOR_FG_WHITE) << line.substr(final_offset);
        }

        source_line << esc(2, ATTR_RESET);
        highlight_line << esc(2, ATTR_RESET);
    }

    std::cerr << source_line.str() << "\n";
    std::cerr << highlight_line.str() << "\n";

    constexpr auto BOX_DRAWINGS_LIGHT_VERTICAL
        [[maybe_unused]] = std::string_view{"│"};  // u+2502
    constexpr auto BOX_DRAWINGS_LIGHT_UP_AND_RIGHT
        [[maybe_unused]] = std::string_view{"└"};  // u+2514
    constexpr auto BOX_DRAWINGS_LIGHT_ARC_UP_AND_RIGHT
        [[maybe_unused]] = std::string_view{"╰"};  // u+2570
    constexpr auto BOX_DRAWINGS_LIGHT_LEFT
        [[maybe_unused]] = std::string_view{"╴"};  // u+2574

    if (not e.aside().empty()) {
        std::cerr << std::string(ERROR_MARKER_SIZE, ' ')
                  << std::string(line_no_width, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.aside_character(), ' ')
                  << '|' << esc(2, ATTR_RESET) << "\n";
        std::cerr << std::string(ERROR_MARKER_SIZE, ' ')
                  << std::string(line_no_width, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.aside_character(), ' ')
                  << e.aside() << esc(2, ATTR_RESET) << "\n";
        std::cerr << esc(2, COLOR_FG_CYAN)
                  << std::string(ERROR_MARKER_SIZE, ' ')
                  << std::string(line_no_width, ' ') << SEPARATOR_ASIDE
                  << esc(2, ATTR_RESET) << "\n";
    }

    {
        std::cerr << std::string(ERROR_MARKER_SIZE, ' ')
                  << std::setw(line_no_width) << (e.line() + 2)
                  << SEPARATOR_SOURCE
                  << view_line_after(source_text, e.location()) << "\n";
    }

    /*
     * The separator to put some space between the source code dump,
     * highlight, etc and the error message.
     */
    std::cerr << std::string(ERROR_MARKER_SIZE, ' ')
              << std::string(line_no_width, ' ') << SEPARATOR_SOURCE << "\n";
}
auto display_error_and_exit
    [[noreturn]] (std::filesystem::path source_path,
                  std::string_view source_text,
                  viua::libs::errors::compile_time::Error const& e) -> void
{
    auto line_no_width = std::max(e.line(), e.line() + 1);
    for (auto const& each : e.fallout) {
        line_no_width =
            std::max(line_no_width, std::max(each.line(), each.line() + 1));
    }
    line_no_width = std::to_string(line_no_width).size();

    display_error(source_path, source_text, line_no_width, e);
    for (auto const& each : e.fallout) {
        display_error(source_path, source_text, line_no_width, each);
    }
    exit(1);
}

auto display_error_in_function(std::filesystem::path const source_path,
                               viua::libs::errors::compile_time::Error const& e,
                               std::string_view const fn_name) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    std::cerr
        << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
        << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1) << esc(2, ATTR_RESET)
        << ':' << esc(2, COLOR_FG_WHITE) << (e.character() + 1)
        << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED) << "error"
        << esc(2, ATTR_RESET) << ": in function " << esc(2, COLOR_FG_WHITE)
        << fn_name << esc(2, ATTR_RESET) << ":\n";
}
}  // namespace viua::libs::stage
