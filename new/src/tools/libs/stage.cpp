/*
 *  Copyright (C) 2022 Marek Marecki
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

#include <iostream>

#include <viua/libs/stage.h>
#include <viua/support/tty.h>

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
    -> std::vector<std::tuple<bool, size_t, size_t>>
{
    /*
     * In case of synthesized lexemes, there may be several of them that have
     * the same offset. Naively including all of them in spans produces... weird
     * results when the output is formatted for an error report.
     *
     * Let's only use the first span at a given offset.
     */
    auto seen_offsets = std::set<size_t>{};

    auto cooked = std::vector<std::tuple<bool, size_t, size_t>>{};
    if (std::get<1>(raw.front()) != 0) {
        cooked.emplace_back(false, 0, raw.front().first);
        seen_offsets.insert(0);
    }
    for (auto const& each : raw) {
        auto const& [hl, offset, size] = cooked.back();
        if (auto const want = (offset + size); want < each.first) {
            if (not seen_offsets.contains(want)) {
                cooked.emplace_back(false, want, (each.first - want));
                seen_offsets.insert(want);
            }
        }
        if (not seen_offsets.contains(each.first)) {
            cooked.emplace_back(true, each.first, each.second);
            seen_offsets.insert(each.first);
        }
    }

    return cooked;
}
auto display_error_and_exit
    [[noreturn]] (std::filesystem::path source_path,
                  std::string_view source_text,
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
    auto const LINE_NO_WIDTH =
        std::to_string(std::max(e.line(), e.line() + 1)).size();

    /*
     * The separator to put some space between the command and the error
     * report.
     */
    std::cerr << std::string(ERROR_MARKER.size(), ' ')
              << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE << "\n";

    if (e.line()) {
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::setw(LINE_NO_WIDTH) << e.line() << SEPARATOR_SOURCE
                  << view_line_before(source_text, e.location()) << "\n";
    }

    auto source_line    = std::ostringstream{};
    auto highlight_line = std::ostringstream{};

    if constexpr (false) {
        auto line = view_line_of(source_text, e.location());

        source_line << esc(2, COLOR_FG_RED) << ERROR_MARKER
                    << std::setw(LINE_NO_WIDTH) << (e.line() + 1)
                    << esc(2, COLOR_FG_WHITE) << SEPARATOR_SOURCE;
        highlight_line << std::string(ERROR_MARKER.size(), ' ')
                       << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE;

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
                    << std::setw(LINE_NO_WIDTH) << (e.line() + 1)
                    << esc(2, COLOR_FG_WHITE) << SEPARATOR_SOURCE;
        highlight_line << std::string(ERROR_MARKER.size(), ' ')
                       << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE;

        auto const spans = cook_spans(e.spans());
        for (auto const& each : spans) {
            auto const& [hl, offset, size] = each;
            if (hl and offset == e.character()) {
                source_line << esc(2, COLOR_FG_RED_1);
                highlight_line << esc(2, COLOR_FG_RED_1) << '^'
                               << esc(2, COLOR_FG_RED)
                               << std::string(size - 1, '~');
            } else if (hl) {
                source_line << esc(2, COLOR_FG_RED);
                highlight_line << esc(2, COLOR_FG_RED)
                               << std::string(size, '~');
            } else {
                source_line << esc(2, COLOR_FG_WHITE);
                highlight_line << std::string(size, ' ');
            }
            source_line << line.substr(offset, size);
        }
        source_line << esc(2, COLOR_FG_WHITE)
                    << line.substr(std::get<1>(spans.back())
                                   + std::get<2>(spans.back()));

        source_line << esc(2, ATTR_RESET);
        highlight_line << esc(2, ATTR_RESET);
    }

    std::cerr << source_line.str() << "\n";
    std::cerr << highlight_line.str() << "\n";

    if (not e.aside().empty()) {
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.character(), ' ') << '|'
                  << esc(2, ATTR_RESET) << "\n";
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.character(), ' ')
                  << e.aside() << esc(2, ATTR_RESET) << "\n";
        std::cerr << esc(2, COLOR_FG_CYAN)
                  << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_ASIDE
                  << esc(2, ATTR_RESET) << "\n";
    }

    {
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::setw(LINE_NO_WIDTH) << (e.line() + 2)
                  << SEPARATOR_SOURCE
                  << view_line_after(source_text, e.location()) << "\n";
    }

    /*
     * The separator to put some space between the source code dump,
     * highlight, etc and the error message.
     */
    std::cerr << std::string(ERROR_MARKER.size(), ' ')
              << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE << "\n";

    std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
              << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
              << (e.line() + 1) << esc(2, ATTR_RESET) << ':'
              << esc(2, COLOR_FG_WHITE) << (e.character() + 1)
              << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED) << "error"
              << esc(2, ATTR_RESET) << ": " << e.str() << "\n";

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
