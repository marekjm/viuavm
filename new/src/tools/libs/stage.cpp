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

#include <elf.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>
#include <string_view>

#include <viua/libs/assembler.h>
#include <viua/libs/stage.h>
#include <viua/support/tty.h>
#include <viua/support/string.h>

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
    -> std::vector<std::tuple<bool, size_t, size_t>>
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
    auto cooked = std::vector<std::tuple<bool, size_t, size_t>>{};
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
        cooked.emplace_back(false, 0, raw.front().first);
        seen_offsets.insert(0);
    } else {
        auto const& each = raw.front();
        cooked.emplace_back(true, each.first, each.second);
        seen_offsets.insert(each.first);
    }

    for (auto const& each : raw) {
        /*
         * Check if we have a gap between last cooked span and the next raw one.
         * In the final cooked span vector the MUST BE NO GAPS so we have to
         * fill any that are found. Of course, gaps should not be highlighted.
         */
        auto const& [hl, offset, size] = cooked.back();
        if (auto const want = (offset + size); want < each.first) {
            if (not seen_offsets.contains(want)) {
                cooked.emplace_back(false, want, (each.first - want));
                seen_offsets.insert(want);
            }
        }

        /*
         * Last, but not least, cook the actual raw span.
         */
        if (not seen_offsets.contains(each.first)) {
            cooked.emplace_back(true, each.first, each.second);
            seen_offsets.insert(each.first);
        }
    }

    return cooked;
}

auto display_error(std::filesystem::path source_path,
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
                  << SEPARATOR_ASIDE << std::string(e.aside_character(), ' ')
                  << '|' << esc(2, ATTR_RESET) << "\n";
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.aside_character(), ' ')
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
}
auto display_error_and_exit
    [[noreturn]] (std::filesystem::path source_path,
                  std::string_view source_text,
                  viua::libs::errors::compile_time::Error const& e) -> void
{
    display_error(source_path, source_text, e);
    for (auto const& each : e.fallout) {
        display_error(source_path, source_text, each);
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

auto save_string_to_strtab(std::vector<uint8_t>& tab,
                           std::string_view const data) -> size_t
{
    {
        /*
         * Scan the strings table to see if the requested data is already there.
         * There is no reason to store extra copies of the same value in .rodata
         * so let's deduplicate.
         *
         * FIXME This is ridiculously slow. Maybe add some cache instead of
         * linearly browsing through the whole table every time?
         */
        auto i = size_t{0};
        while ((i + data.size()) < tab.size()) {
            auto const existing = std::string_view{
                reinterpret_cast<char const*>(tab.data() + i), data.size()};

            auto const content_matches = (existing == data);
            auto const nul_matches = (*(tab.data() + i + data.size()) == '\0');
            if (content_matches and nul_matches) {
                return i;
            }

            ++i;
        }
    }

    auto const saved_location = tab.size();

    std::copy(data.begin(), data.end(), std::back_inserter(tab));
    tab.push_back('\0');

    return saved_location;
}
auto save_buffer_to_rodata(std::vector<uint8_t>& strings,
                           std::string_view const data) -> size_t
{
    {
        /*
         * Scan the strings table to see if the requested data is already there.
         * There is no reason to store extra copies of the same value in .rodata
         * so let's deduplicate.
         *
         * FIXME This is ridiculously slow. Maybe add some cache instead of
         * linearly browsing through the whole table every time?
         */
        auto i = size_t{0};
        while (i < strings.size()) {
            auto data_size = uint64_t{};
            memcpy(&data_size, strings.data() + i, sizeof(data_size));
            data_size = le64toh(data_size);

            i += sizeof(data_size);

            auto const existing = std::string_view{
                reinterpret_cast<char const*>(strings.data() + i), data_size};
            if (existing == data) {
                return i;
            }

            i += data_size;
        }
    }

    auto const data_size = htole64(static_cast<uint64_t>(data.size()));
    strings.resize(strings.size() + sizeof(data_size));
    memcpy((strings.data() + strings.size() - sizeof(data_size)),
           &data_size,
           sizeof(data_size));

    auto const saved_location = strings.size();
    std::copy(data.begin(), data.end(), std::back_inserter(strings));

    return saved_location;
}
auto record_symbol(std::string name,
                   Elf64_Sym const symbol,
                   std::vector<Elf64_Sym>& symbol_table,
                   std::map<std::string, size_t>& symbol_map) -> size_t
{
    auto const sym_ndx          = symbol_table.size();
    symbol_map[std::move(name)] = sym_ndx;
    symbol_table.push_back(symbol);
    return sym_ndx;
}
auto cook_long_immediates(viua::libs::parser::ast::Instruction insn,
                          std::vector<uint8_t>& rodata_buf,
                          std::vector<Elf64_Sym>& symbol_table,
                          std::map<std::string, size_t>& symbol_map)
    -> std::vector<viua::libs::parser::ast::Instruction>
{
    auto cooked = std::vector<viua::libs::parser::ast::Instruction>{};

    // FIXME saved_at is the index into the symbol table. This means that even
    // anonymous symbols must be saved into .symtab to be processed by the
    // linker.

    if (insn.opcode == "atom" or insn.opcode == "g.atom") {
        auto const lx = insn.operands.back().ingredients.front();
        auto s        = lx.text;
        auto saved_at = size_t{0};
        if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
            s = s.substr(1, s.size() - 2);
            s = viua::support::string::unescape(s);

            auto const value_off = save_buffer_to_rodata(rodata_buf, s);

            auto symbol     = Elf64_Sym{};
            symbol.st_name  = 0; /* anonymous symbol */
            symbol.st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
            symbol.st_other = STV_DEFAULT;

            symbol.st_value = value_off;
            symbol.st_size  = s.size();

            /*
             * Section header table index (see elf(5) for st_shndx) is filled
             * out later, since at this point we do not have this information.
             *
             * For variables it will be the index of .rodata.
             * For functions it will be the index of .text.
             */
            symbol.st_shndx = 0;

            saved_at = record_symbol("", symbol, symbol_table, symbol_map);
        } else if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
            auto s = lx.text;

            auto const value_off = save_buffer_to_rodata(rodata_buf, s);

            auto symbol     = Elf64_Sym{};
            symbol.st_name  = 0; /* anonymous symbol */
            symbol.st_info  = ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);
            symbol.st_other = STV_DEFAULT;

            symbol.st_value = value_off;
            symbol.st_size  = s.size();

            /*
             * Section header table index (see elf(5) for st_shndx) is filled
             * out later, since at this point we do not have this information.
             *
             * For variables it will be the index of .rodata.
             * For functions it will be the index of .text.
             */
            symbol.st_shndx = 0;

            saved_at = record_symbol("", symbol, symbol_table, symbol_map);
        } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
            auto const label = insn.operands.back().ingredients.back();
            try {
                saved_at = symbol_map.at(label.text);
                /* saved_at = symbol_table.at(saved_at).st_value; */
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{label, Cause::Unknown_label, label.text};
                e.add(lx);

                using viua::support::string::levenshtein_filter;
                auto misspell_candidates =
                    levenshtein_filter(label.text, symbol_map);
                if (not misspell_candidates.empty()) {
                    using viua::support::string::levenshtein_best;
                    auto best_candidate =
                        levenshtein_best(label.text,
                                         misspell_candidates,
                                         (label.text.size() / 2));
                    if (best_candidate.second != label.text) {
                        viua::libs::parser::did_you_mean(e,
                                                         best_candidate.second);
                    }
                }

                throw e;
            }
        } else {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto e = Error{lx, Cause::Invalid_operand}.aside(
                "expected string literal, atom literal, or a label "
                "reference");

            if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                viua::libs::parser::did_you_mean(e, '@' + lx.text);
            }

            throw e;
        }

        auto synth = viua::libs::parser::ast::Instruction{};
        {
            synth.opcode         = insn.opcode;
            synth.opcode.text    = "g.li";
            synth.physical_index = insn.physical_index;

            synth.operands.push_back(insn.operands.front());
            synth.operands.push_back(insn.operands.back());
            synth.operands.back().ingredients.front().text =
                std::to_string(saved_at) + 'u';
        }
        expand_li(cooked, synth, true);

        insn.operands.pop_back();
        cooked.push_back(std::move(insn));
    } else if (insn.opcode == "arodp" or insn.opcode == "g.arodp") {
        auto const lx = insn.operands.back().ingredients.front();
        auto saved_at = size_t{0};
        if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
            auto s = lx.text;
            s      = s.substr(1, s.size() - 2);
            s      = viua::support::string::unescape(s);
            saved_at =
                (save_buffer_to_rodata(rodata_buf, s) - sizeof(uint64_t));
        } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
            auto const label = insn.operands.back().ingredients.back();
            try {
                saved_at = symbol_map.at(label.text);
                saved_at = symbol_table.at(saved_at).st_value;
                saved_at = (saved_at - sizeof(uint64_t));
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{label, Cause::Unknown_label, label.text};
                e.add(lx);

                using viua::support::string::levenshtein_filter;
                auto misspell_candidates =
                    levenshtein_filter(label.text, symbol_map);
                if (not misspell_candidates.empty()) {
                    using viua::support::string::levenshtein_best;
                    auto best_candidate =
                        levenshtein_best(label.text,
                                         misspell_candidates,
                                         (label.text.size() / 2));
                    if (best_candidate.second != label.text) {
                        viua::libs::parser::did_you_mean(e,
                                                         best_candidate.second);
                    }
                }

                throw e;
            }
        } else {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto e = Error{lx, Cause::Invalid_operand}.aside(
                "expected string literal, or a label reference");

            if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                viua::libs::parser::did_you_mean(e, '@' + lx.text);
            }

            throw e;
        }

        auto synth           = viua::libs::parser::ast::Instruction{};
        synth.opcode         = insn.opcode;
        synth.physical_index = insn.physical_index;

        synth.operands.push_back(insn.operands.front());
        synth.operands.push_back(insn.operands.back());
        synth.operands.back().ingredients.front().text =
            std::to_string(saved_at) + 'u';

        cooked.push_back(synth);
    } else if (insn.opcode == "double" or insn.opcode == "g.double") {
        constexpr auto SIZE_OF_DOUBLE_PRECISION_FLOAT = size_t{8};
        auto f = std::stod(insn.operands.back().ingredients.front().text);
        auto s = std::string(SIZE_OF_DOUBLE_PRECISION_FLOAT, '\0');
        memcpy(s.data(), &f, SIZE_OF_DOUBLE_PRECISION_FLOAT);
        auto const saved_at = save_buffer_to_rodata(rodata_buf, s);

        auto synth = viua::libs::parser::ast::Instruction{};
        {
            synth.opcode         = insn.opcode;
            synth.opcode.text    = "g.li";
            synth.physical_index = insn.physical_index;

            synth.operands.push_back(insn.operands.front());
            synth.operands.push_back(insn.operands.back());
            synth.operands.back().ingredients.front().text =
                std::to_string(saved_at) + 'u';
        }
        expand_li(cooked, synth, true);

        insn.operands.pop_back();
        cooked.push_back(std::move(insn));
    } else {
        cooked.push_back(std::move(insn));
    }

    return cooked;
}

using namespace viua::libs::parser;
auto expand_delete(std::vector<ast::Instruction>& cooked,
                   ast::Instruction const& raw) -> void
{
    using namespace std::string_literals;
    auto synth        = ast::Instruction{};
    synth.opcode      = raw.opcode;
    synth.opcode.text = (raw.opcode.text.find("g.") == 0 ? "g." : "") + "move"s;
    synth.physical_index = raw.physical_index;

    synth.operands.push_back(raw.operands.front());
    synth.operands.push_back(raw.operands.front());

    synth.operands.front().ingredients.front().text = "void";
    synth.operands.front().ingredients.resize(1);

    cooked.push_back(synth);
}
auto expand_li(std::vector<ast::Instruction>& cooked,
               ast::Instruction const& each,
               bool const force_full) -> void
{
    auto const& raw_value = each.operands.at(1).ingredients.front();
    auto value            = uint64_t{};
    try {
        if (raw_value.text.find("0x") == 0) {
            value = std::stoull(raw_value.text, nullptr, 16);
        } else if (raw_value.text.find("0o") == 0) {
            value = std::stoull(raw_value.text, nullptr, 8);
        } else if (raw_value.text.find("0b") == 0) {
            value = std::stoull(raw_value.text, nullptr, 2);
        } else {
            value = std::stoull(raw_value.text);
        }
    } catch (std::invalid_argument const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{raw_value, Cause::Invalid_operand, "expected integer"}.add(
            each.opcode);
    }

    using viua::libs::assembler::to_loading_parts_unsigned;
    auto const [hi, lo]  = to_loading_parts_unsigned(value);
    auto const is_greedy = (each.opcode.text.find("g.") == 0);
    auto const full_form = each.operands.at(1).has_attr("full") or force_full;

    auto const is_unsigned = (raw_value.text.back() == 'u');

    constexpr auto LOW_24   = uint32_t{0x00ffffff};
    auto const fits_in_addi = ((lo & LOW_24) == lo);

    auto const needs_leader = full_form or hi or (not fits_in_addi);

    /*
     * When loading immediates we have several cases to consider:
     *
     *  - the immediate is short ie, occupies only lower 24-bits and thus fits
     *    fully in the immediate of an ADDI instruction
     *  - the immediate is long ie, has any of the high 40 bits set
     *
     * In the first case we could be done with a single ADDI, and (if we are not
     * loading addresses and need to take the pessimistic route to accommodate
     * the linker) sometimes we can.
     *
     * However, in most cases the LI pseudoinstruction must be expanded into two
     * real instructions:
     *
     *  - LUI to load the high word
     *  - LLI to load the low word
     *
     * LUI is necessary even if the long immediate being loaded only has lower
     * 32 bits set, because LLI needs a value to be already present in the
     * output register to determine the signedness of its output.
     *
     * In any case, for long immediates the sequence of
     *
     *      g.lui $x, <high-word>
     *      lli $x, <low-word>
     *
     * is emitted which is cheap and executed without releasing the virtual CPU.
     */
    if (needs_leader) {
        using namespace std::string_literals;
        auto synth        = each;
        synth.opcode.text = ((lo or is_greedy) ? "g.lui" : "lui");
        if (is_unsigned) {
            synth.opcode.text += 'u';
        }

        auto hi_literal = std::array<char, 2 + 8 + 1>{"0x"};
        std::to_chars(hi_literal.begin() + 2, hi_literal.end(), hi, 16);

        synth.operands.at(1).ingredients.front().text =
            std::string{hi_literal.data()};
        cooked.push_back(synth);
    }

    /*
     * In the second step we use LLI to load the lower word if we are dealing
     * with a long immediate; or ADDI if we have a short immediate to deal with.
     */
    if (needs_leader) {
        auto synth           = ast::Instruction{};
        synth.opcode         = each.opcode;
        synth.opcode.text    = (is_greedy ? "g.lli" : "lli");
        synth.physical_index = each.physical_index;

        auto const& lx = each.operands.front().ingredients.at(1);

        using viua::libs::lexer::TOKEN;
        {
            auto dst = ast::Operand{};
            dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
            dst.ingredients.push_back(lx.make_synth(
                std::to_string(std::stoull(lx.text)), TOKEN::LITERAL_INTEGER));
            dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
            dst.ingredients.push_back(lx.make_synth("l", TOKEN::LITERAL_ATOM));

            synth.operands.push_back(dst);
        }
        {
            auto immediate = ast::Operand{};
            immediate.ingredients.push_back(
                lx.make_synth(std::to_string(lo), TOKEN::LITERAL_INTEGER));

            synth.operands.push_back(immediate);
        }

        cooked.push_back(synth);
    } else {
        using namespace std::string_literals;
        auto synth           = ast::Instruction{};
        synth.opcode         = each.opcode;
        synth.opcode.text    = (is_greedy ? "g." : "") + "addi"s;
        synth.physical_index = each.physical_index;
        if (is_unsigned) {
            synth.opcode.text += 'u';
        }

        synth.operands.push_back(each.operands.front());

        /*
         * If the first part of the load (the high 36 bits) was zero then it
         * means we don't have anything to add to so the source (left-hand side
         * operand) should be void ie, the default value.
         */
        synth.operands.push_back(each.operands.front());
        synth.operands.back().ingredients.front().text = "void";
        synth.operands.back().ingredients.resize(1);

        synth.operands.push_back(each.operands.back());
        synth.operands.back().ingredients.front().text = std::to_string(lo);

        cooked.push_back(synth);
    }
}
auto expand_if(std::vector<ast::Instruction>& cooked,
               ast::Instruction& each,
               std::map<size_t, size_t> l2p) -> void
{
    /*
     * The address to jump to is not cooked directly into the final
     * instruction sequence because we may have to try several times
     * before getting the "right" instruction sequence. Why? See the
     * comment for teh `branch_target' variable in the loop below.
     *
     * Basically, it boils down to the fact that:
     *
     * - we have to adjust the target instruction sequence based on the
     *   value of the index of the instruction to jump to
     * - we have to adjust the target index also by the difference
     *   between logical and physical instruction count
     */
    auto address_load             = std::vector<ast::Instruction>{};
    auto prev_address_load_length = address_load.size();
    constexpr auto MAX_SIZE_OF_LI = size_t{9};
    do {
        prev_address_load_length = address_load.size();
        address_load.clear();

        auto const branch_target =
            l2p.at(std::stoull(each.operands.back().to_string()));

        auto li = ast::Instruction{};
        {
            li.opcode         = each.opcode;
            li.opcode.text    = "g.li";
            li.physical_index = each.physical_index;

            using viua::libs::lexer::TOKEN;
            auto const& lx = each.operands.front().ingredients.front();
            {
                auto reg = ast::Operand{};
                reg.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                reg.ingredients.push_back(
                    lx.make_synth("253", TOKEN::LITERAL_INTEGER));
                reg.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                reg.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));
                li.operands.push_back(reg);
            }
            {
                auto br = ast::Operand{};
                br.ingredients.push_back(
                    lx.make_synth(std::to_string(branch_target) + 'u',
                                  TOKEN::LITERAL_INTEGER));
                li.operands.push_back(br);
            }
        }

        /*
         * Try cooking the instruction sequence to see what amount of
         * instructions we will get. If we get the same instruction
         * sequence twice - we got our match and the loop can end. The
         * downside of this brute-force algorithm is that we may have to
         * cook the same code twice in some cases.
         */
        expand_li(address_load, li);
    } while (address_load.size() < MAX_SIZE_OF_LI
             and prev_address_load_length != address_load.size());
    std::copy(
        address_load.begin(), address_load.end(), std::back_inserter(cooked));

    using viua::libs::lexer::TOKEN;
    auto reg = each.operands.back();
    each.operands.pop_back();

    auto const lx = reg.ingredients.front();
    reg.ingredients.pop_back();

    reg.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
    reg.ingredients.push_back(lx.make_synth("253", TOKEN::LITERAL_INTEGER));
    reg.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
    reg.ingredients.push_back(lx.make_synth("l", TOKEN::LITERAL_ATOM));

    each.operands.push_back(reg);
    cooked.push_back(std::move(each));
}
auto expand_memory_access(std::vector<ast::Instruction>& cooked,
                          ast::Instruction const& raw) -> void
{
    using namespace std::string_literals;
    auto synth = ast::Instruction{};

    auto opcode_view = std::string_view{raw.opcode.text};
    auto greedy      = opcode_view.starts_with("g.");
    if (greedy) {
        opcode_view.remove_prefix(2);
    }

    synth.opcode         = raw.opcode;
    synth.opcode.text    = opcode_view;
    synth.opcode.text[1] = 'm';
    if (opcode_view[0] == 'a') {
        switch (synth.opcode.text.back()) {
        case 'a':
            synth.opcode.text = "ama";
            break;
        case 'd':
            synth.opcode.text = "amd";
            break;
        default:
            abort();
        }
    }
    synth.physical_index = raw.physical_index;

    synth.operands.push_back(raw.operands.at(0));
    synth.operands.push_back(raw.operands.at(0));
    synth.operands.push_back(raw.operands.at(1));
    synth.operands.push_back(raw.operands.at(2));

    auto const unit = opcode_view[0] == 'a' ? opcode_view[2] : opcode_view[1];
    switch (unit) {
    case 'b':
        synth.operands.front().ingredients.front().text = "0";
        break;
    case 'h':
        synth.operands.front().ingredients.front().text = "1";
        break;
    case 'w':
        synth.operands.front().ingredients.front().text = "2";
        break;
    case 'd':
        synth.operands.front().ingredients.front().text = "3";
        break;
    case 'q':
        synth.operands.front().ingredients.front().text = "4";
        break;
    default:
        abort();
    }
    synth.operands.front().ingredients.resize(1);

    if (greedy) {
        synth.opcode.text = ("g." + synth.opcode.text);
    }

    cooked.push_back(synth);
}
auto expand_pseudoinstructions(std::vector<ast::Instruction> raw,
                               std::map<std::string, size_t> const& symbol_map)
    -> std::vector<ast::Instruction>
{
    auto const immediate_signed_arithmetic = std::set<std::string>{
        "addi",
        "g.addi",
        "subi",
        "g.subi",
        "muli",
        "g.muli",
        "divi",
        "g.divi",
    };
    auto const memory_access = std::set<std::string>{
        /*
         * Access
         */
        "sb",
        "lb",
        "sh",
        "lh",
        "sw",
        "lw",
        "sd",
        "ld",
        "sq",
        "lq",

        "g.sb",
        "g.lb",
        "g.sh",
        "g.lh",
        "g.sw",
        "g.lw",
        "g.sd",
        "g.ld",
        "g.sq",
        "g.lq",

        /*
         * Allocation
         */
        "amba",
        "amha",
        "amwa",
        "amda",
        "amqa",
        "ambd",
        "amhd",
        "amwd",
        "amdd",
        "amqd",
    };

    /*
     * We need to maintain accurate mapping of logical to physical instructions
     * because what the programmer sees is not necessarily what the VM will be
     * executing. Pseudoinstructions are expanded into sequences of real
     * instructions and this changes effective addresses (ie, instruction
     * indexes) which are used as branch targets.
     *
     * A single logical instruction can expand into as many as 9 physical
     * instructions! The worst (and most common) such pseudoinstruction is the
     * li -- Loading Integer constants. It is used to store integers in
     * registers, as well as addresses for calls, jumps, and ifs.
     */
    auto physical_ops       = size_t{0};
    auto logical_ops        = size_t{0};
    auto branch_ops_baggage = size_t{0};
    auto l2p                = std::map<size_t, size_t>{};

    /*
     * First, we cook the sequence of raw instructions expanding non-control
     * flow instructions and building the mapping of logical to physical
     * instruction indexes.
     *
     * We do it this way because even an early branch can jump to the very last
     * instruction in a function, but we can't predict the future so we do not
     * know at this point how many PHYSICAL instructions are there between the
     * branch and its target -- this means that we do not know how to adjust the
     * logical target of the branch.
     *
     * What we can do, though, is to accrue the "instruction cost" of expanding
     * the branches and adjust the physical instruction indexes by this value. I
     * think this may be brittle and error-prone but so far it works... Let's
     * mark this part as HERE BE DRAGONS to be easier to find in the future.
     */
    auto cooked = std::vector<ast::Instruction>{};
    for (auto& each : raw) {
        physical_ops = each.physical_index;
        l2p.insert({physical_ops, logical_ops + branch_ops_baggage});

        // FIXME remove checking for "g.li" here to test errors with synthesized
        // instructions
        if (each.opcode == "li" or each.opcode == "g.li") {
            expand_li(cooked, each);
        } else if (each.opcode == "delete" or each.opcode == "g.delete") {
            expand_delete(cooked, each);
        } else if (each.opcode == "call" or each.opcode == "actor"
                   or each.opcode == "g.actor") {
            /*
             * Call instructions expansion is simple.
             *
             * First, a li pseudoinstruction containing the offset of the
             * function in the function table is synthesized and expanded. The
             * call pseudoinstruction is then replaced by a real call
             * instruction in D format - first register tells it where to put
             * the return value, and the second tells it from which register to
             * take the function table offset value.
             *
             * If the return register is not void, it is used by the li
             * pseudoinstruction as base. If it is void, the li
             * pseudoinstruction has a base register allocated from a free
             * range.
             *
             * In effect, the following pseudoinstruction:
             *
             *      call $1, foo
             *
             * ...is expanded into this sequence:
             *
             *      li $1, fn_tbl_offset(foo)
             *      call $1, $1
             */
            auto const ret = each.operands.front();
            auto fn_offset = ret;
            if (ret.ingredients.front() == "void") {
                /*
                 * If the return register is void we need a completely synthetic
                 * register to store the function offset. The li
                 * pseudoinstruction uses at most three registers to do its job
                 * so we can use register 253 as the base to avoid disturbing
                 * user code.
                 */
                fn_offset = ast::Operand{};

                using viua::libs::lexer::TOKEN;
                auto const& lx = ret.ingredients.front();
                fn_offset.ingredients.push_back(
                    lx.make_synth("$", TOKEN::RA_DIRECT));
                fn_offset.ingredients.push_back(
                    lx.make_synth("253", TOKEN::LITERAL_INTEGER));
                fn_offset.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                fn_offset.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));
            }

            /*
             * Synthesize loading the function offset first. It will emit a
             * sequence of instructions that will load the offset of the
             * function, which will then be used by the call instruction to
             * invoke the function.
             */
            auto li = ast::Instruction{};
            {
                li.opcode      = each.opcode;
                li.opcode.text = "g.li";

                li.operands.push_back(fn_offset);
                li.operands.push_back(ast::Operand{});

                using viua::libs::lexer::TOKEN;

                auto const fn_id = each.operands.back().ingredients.front();
                auto const fn_name = (fn_id.token == TOKEN::LITERAL_STRING)
                    ? fn_id.text.substr(1, fn_id.text.size() - 2)
                    : fn_id.text;
                if (symbol_map.count(fn_name) == 0) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{fn_id, Cause::Call_to_undefined_function,
                        quote_fancy(fn_name)};
                }

                auto const fn_off = symbol_map.at(fn_name);
                li.operands.back().ingredients.push_back(fn_id.make_synth(
                    std::to_string(fn_off) + 'u',
                    TOKEN::LITERAL_INTEGER));

                li.physical_index = each.physical_index;
            }
            expand_li(cooked, li, true);

            /*
             * Then, synthesize the actual call instruction. This means
             * simply replacing the function name with the register
             * containing its loaded offset, ie, changing this code:
             *
             *      call $42, foo
             *
             * to this:
             *
             *      call $42, $42
             */
            auto call = ast::Instruction{};
            {
                call.opcode = each.opcode;
                call.operands.push_back(ret);
                call.operands.push_back(fn_offset);
                call.physical_index = each.physical_index;
            }
            cooked.push_back(call);
        } else if (each.opcode == "return") {
            if (each.operands.empty()) {
                /*
                 * Return instruction takes a single register operand, but this
                 * operand can be omitted. As in C, if omitted it defaults to
                 * void.
                 */
                auto operand = ast::Operand{};
                operand.ingredients.push_back(each.opcode);
                operand.ingredients.back().text = "void";
                each.operands.push_back(operand);
            }

            cooked.push_back(std::move(each));
        } else if (immediate_signed_arithmetic.count(each.opcode.text)) {
            if (each.operands.back().ingredients.back().text.back() == 'u') {
                each.opcode.text += 'u';
            }
            cooked.push_back(std::move(each));
        } else if (memory_access.count(each.opcode.text)) {
            expand_memory_access(cooked, each);
        } else {
            /*
             * Real instructions should be pushed without any modification.
             */
            cooked.push_back(std::move(each));
        }

        logical_ops += (cooked.size() - logical_ops);

        using viua::libs::assembler::li_cost;
        if (each.opcode == "if") {
            try {
                branch_ops_baggage +=
                    li_cost(std::stoull(each.operands.back().to_string()));
            } catch (std::invalid_argument const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{each.operands.back().ingredients.front(),
                               Cause::Invalid_operand};
                std::for_each(each.operands.back().ingredients.begin() + 1,
                              each.operands.back().ingredients.end(),
                              [&e](auto const& ing) -> void { e.add(ing); });
                e.aside("must be an offset into the function's body");
                throw e;
            }
        } else if (each.opcode == "jump") {
            branch_ops_baggage +=
                li_cost(std::stoull(each.operands.back().to_string()));
        }
    }

    auto baked = std::vector<ast::Instruction>{};
    for (auto& each : cooked) {
        if (each.opcode == "if") {
            expand_if(baked, each, l2p);
        } else if (each.opcode == "jump") {
            /*
             * Jumps can be safely rewritten as ifs with a void condition
             * register. There is no reason not to do this, frankly, since
             * taking a void register as an operand is perfectly valid and an
             * established practice in the ISA -- void acts as a sort of a
             * default-value register. Thanks RISC-V for the idea of hardwired
             * r0!
             */
            auto as_if           = ast::Instruction{};
            as_if.opcode         = each.opcode;
            as_if.opcode.text    = "if";
            as_if.physical_index = each.physical_index;

            {
                /*
                 * The condition operand for the emitted if is just a void. This
                 * is interpreted by the VM as a default value which in this
                 * case means "take the branch".
                 */
                auto condition = ast::Operand{};
                auto const& lx = each.operands.front().ingredients.front();
                condition.ingredients.push_back(
                    lx.make_synth("void", viua::libs::lexer::TOKEN::RA_VOID));
                as_if.operands.push_back(condition);
            }

            /*
             * if is a D-format instruction so it requires two operands. The
             * second one is the address to which the branch should be taken. We
             * have this readily available (in fact, it was provided by the
             * programmer) so let's just pass it on.
             */
            as_if.operands.push_back(each.operands.front());

            expand_if(baked, as_if, l2p);
        } else {
            /*
             * Real instructions should be pushed without any modification.
             */
            baked.push_back(std::move(each));
        }
    }

    return baked;
}

auto operand_or_throw(viua::libs::parser::ast::Instruction const& insn,
                      size_t const index)
    -> viua::libs::parser::ast::Operand const&
{
    try {
        return insn.operands.at(index);
    } catch (std::out_of_range const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{insn.opcode,
                    Cause::Too_few_operands,
                    ("operand " + std::to_string(index) + " not found")};
    }
}
auto emit_instruction(viua::libs::parser::ast::Instruction const insn)
    -> viua::arch::instruction_type
{
    using viua::arch::opcode_type;
    using viua::arch::ops::FORMAT;
    using viua::arch::ops::OPCODE;
    using viua::arch::ops::FORMAT_MASK;

    auto opcode = opcode_type{};
    try {
        opcode = insn.parse_opcode();
    } catch (std::invalid_argument const&) {
        auto const e = insn.opcode;

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{e, Cause::Unknown_opcode, e.text};
    }
    auto format = static_cast<FORMAT>(opcode & FORMAT_MASK);
    switch (format) {
    case FORMAT::N:
        return static_cast<uint64_t>(opcode);
    case FORMAT::T:
        return viua::arch::ops::T{opcode,
                                  operand_or_throw(insn, 0).make_access(),
                                  operand_or_throw(insn, 1).make_access(),
                                  operand_or_throw(insn, 2).make_access()}
            .encode();
    case FORMAT::D:
        return viua::arch::ops::D{opcode,
                                  operand_or_throw(insn, 0).make_access(),
                                  operand_or_throw(insn, 1).make_access()}
            .encode();
    case FORMAT::S:
        return viua::arch::ops::S{opcode,
                                  operand_or_throw(insn, 0).make_access()}
            .encode();
    case FORMAT::F:
    {
        auto const raw = operand_or_throw(insn, 1).ingredients.front().text;
        auto val       = static_cast<uint32_t>(std::stoul(raw, nullptr, 0));
        if (static_cast<OPCODE>(opcode) == OPCODE::FLOAT) {
            auto tmp = std::stof(raw);
            memcpy(&val, &tmp, sizeof(val));
        }
        return viua::arch::ops::F{
            opcode, operand_or_throw(insn, 0).make_access(), val}
            .encode();
    }
    case FORMAT::E:
        return viua::arch::ops::E{
            opcode,
            operand_or_throw(insn, 0).make_access(),
            std::stoull(
                operand_or_throw(insn, 1).ingredients.front().text, nullptr, 0)}
            .encode();
    case FORMAT::R:
    {
        auto const imm = insn.operands.back().ingredients.front();
        auto const is_unsigned =
            (static_cast<opcode_type>(opcode) & viua::arch::ops::UNSIGNED);
        if (is_unsigned and imm.text.at(0) == '-'
            and (imm.text != "-1" and imm.text != "-1u")) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{imm,
                        Cause::Value_out_of_range,
                        "signed integer used for unsigned immediate"}
                .note("the only signed value allowed in this context "
                      "is -1, and\n"
                      "it is used a symbol for maximum unsigned "
                      "immediate value");
        }
        if ((not is_unsigned) and imm.text.back() == 'u') {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{imm,
                        Cause::Value_out_of_range,
                        "unsigned integer used for signed immediate"};
        }
        try {
            return viua::arch::ops::R{
                opcode,
                insn.operands.at(0).make_access(),
                insn.operands.at(1).make_access(),
                (is_unsigned ? static_cast<uint32_t>(std::stoul(imm.text))
                             : static_cast<uint32_t>(std::stoi(imm.text)))}
                .encode();
        } catch (std::invalid_argument const&) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            // FIXME make the error more precise, maybe encapsulate
            // just the immediate operand conversion
            throw Error{imm,
                        Cause::Invalid_operand,
                        "expected integer as immediate operand"};
        }
    }
    case FORMAT::M:
    {
        auto const unit = insn.operands.front().ingredients.front();
        auto const off  = insn.operands.back().ingredients.front();

        return viua::arch::ops::M{opcode,
                                  insn.operands.at(1).make_access(),
                                  insn.operands.at(2).make_access(),
                                  static_cast<uint16_t>(std::stoull(off.text)),
                                  static_cast<uint8_t>(std::stoull(unit.text))}
            .encode();
    }
    default:
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{
            insn.opcode, Cause::Unknown_opcode, "cannot emit instruction"};
    }
}
}  // namespace viua::libs::stage
