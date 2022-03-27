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

#include <string.h>

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

auto save_string(std::vector<uint8_t>& strings, std::string_view const data)
    -> size_t
{
    auto const data_size = htole64(static_cast<uint64_t>(data.size()));
    strings.resize(strings.size() + sizeof(data_size));
    memcpy((strings.data() + strings.size() - sizeof(data_size)),
           &data_size,
           sizeof(data_size));

    auto const saved_location = strings.size();
    std::copy(data.begin(), data.end(), std::back_inserter(strings));

    return saved_location;
}
auto cook_long_immediates(viua::libs::parser::ast::Instruction insn,
                          std::vector<uint8_t>& strings_table,
                          std::map<std::string, size_t>& var_offsets) -> std::vector<viua::libs::parser::ast::Instruction>
{
    auto cooked = std::vector<viua::libs::parser::ast::Instruction>{};

    if (insn.opcode == "atom" or insn.opcode == "g.atom") {
        auto const lx = insn.operands.back().ingredients.front();
        auto s        = lx.text;
        auto saved_at = size_t{0};
        if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
            s        = s.substr(1, s.size() - 2);
            s        = viua::support::string::unescape(s);
            saved_at = save_string(strings_table, s);
        } else if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
            auto s   = lx.text;
            saved_at = save_string(strings_table, s);
        } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
            auto const label = insn.operands.back().ingredients.back();
            try {
                saved_at = var_offsets.at(label.text);
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{label, Cause::Unknown_label, label.text};
                e.add(lx);

                using viua::support::string::levenshtein_filter;
                auto misspell_candidates =
                    levenshtein_filter(label.text, var_offsets);
                if (not misspell_candidates.empty()) {
                    using viua::support::string::levenshtein_best;
                    auto best_candidate =
                        levenshtein_best(label.text,
                                         misspell_candidates,
                                         (label.text.size() / 2));
                    if (best_candidate.second != label.text) {
                        viua::libs::parser::did_you_mean(e, best_candidate.second);
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

        auto synth           = viua::libs::parser::ast::Instruction{};
        synth.opcode         = insn.opcode;
        synth.opcode.text    = "g.li";
        synth.physical_index = insn.physical_index;

        synth.operands.push_back(insn.operands.front());
        synth.operands.push_back(insn.operands.back());
        synth.operands.back().ingredients.front().text =
            std::to_string(saved_at) + 'u';

        cooked.push_back(synth);

        insn.operands.pop_back();
        cooked.push_back(std::move(insn));
    } else if (insn.opcode == "string" or insn.opcode == "g.string") {
        auto const lx = insn.operands.back().ingredients.front();
        auto saved_at = size_t{0};
        if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
            auto s   = lx.text;
            s        = s.substr(1, s.size() - 2);
            s        = viua::support::string::unescape(s);
            saved_at = save_string(strings_table, s);
        } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
            auto const label = insn.operands.back().ingredients.back();
            try {
                saved_at = var_offsets.at(label.text);
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{label, Cause::Unknown_label, label.text};
                e.add(lx);

                using viua::support::string::levenshtein_filter;
                auto misspell_candidates =
                    levenshtein_filter(label.text, var_offsets);
                if (not misspell_candidates.empty()) {
                    using viua::support::string::levenshtein_best;
                    auto best_candidate =
                        levenshtein_best(label.text,
                                         misspell_candidates,
                                         (label.text.size() / 2));
                    if (best_candidate.second != label.text) {
                        viua::libs::parser::did_you_mean(e, best_candidate.second);
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
        synth.opcode.text    = "g.li";
        synth.physical_index = insn.physical_index;

        synth.operands.push_back(insn.operands.front());
        synth.operands.push_back(insn.operands.back());
        synth.operands.back().ingredients.front().text =
            std::to_string(saved_at) + 'u';

        cooked.push_back(synth);

        insn.operands.pop_back();
        cooked.push_back(std::move(insn));
    } else if (insn.opcode == "float" or insn.opcode == "g.float") {
        constexpr auto SIZE_OF_SINGLE_PRECISION_FLOAT = size_t{4};
        auto f =
            std::stof(insn.operands.back().ingredients.front().text);
        auto s = std::string(SIZE_OF_SINGLE_PRECISION_FLOAT, '\0');
        memcpy(s.data(), &f, SIZE_OF_SINGLE_PRECISION_FLOAT);
        auto const saved_at = save_string(strings_table, s);

        auto synth           = viua::libs::parser::ast::Instruction{};
        synth.opcode         = insn.opcode;
        synth.opcode.text    = "g.li";
        synth.physical_index = insn.physical_index;

        synth.operands.push_back(insn.operands.front());
        synth.operands.push_back(insn.operands.back());
        synth.operands.back().ingredients.front().text =
            std::to_string(saved_at) + 'u';

        cooked.push_back(synth);

        insn.operands.pop_back();
        cooked.push_back(std::move(insn));
    } else if (insn.opcode == "double" or insn.opcode == "g.double") {
        constexpr auto SIZE_OF_DOUBLE_PRECISION_FLOAT = size_t{8};
        auto f =
            std::stod(insn.operands.back().ingredients.front().text);
        auto s = std::string(SIZE_OF_DOUBLE_PRECISION_FLOAT, '\0');
        memcpy(s.data(), &f, SIZE_OF_DOUBLE_PRECISION_FLOAT);
        auto const saved_at = save_string(strings_table, s);

        auto synth           = viua::libs::parser::ast::Instruction{};
        synth.opcode         = insn.opcode;
        synth.opcode.text    = "g.li";
        synth.physical_index = insn.physical_index;

        synth.operands.push_back(insn.operands.front());
        synth.operands.push_back(insn.operands.back());
        synth.operands.back().ingredients.front().text =
            std::to_string(saved_at) + 'u';

        cooked.push_back(synth);

        insn.operands.pop_back();
        cooked.push_back(std::move(insn));
    } else {
        cooked.push_back(std::move(insn));
    }

    return cooked;
}

auto operand_or_throw(viua::libs::parser::ast::Instruction const& insn, size_t const index)
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
auto emit_instruction(viua::libs::parser::ast::Instruction const insn) -> viua::arch::instruction_type
{
    using viua::arch::opcode_type;
    using viua::arch::ops::FORMAT;
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
                return
            viua::arch::ops::S{opcode,
                               operand_or_throw(insn, 0).make_access()}
                .encode();
    case FORMAT::F:
        return uint64_t{0};  // FIXME
    case FORMAT::E:
        return
            viua::arch::ops::E{
                opcode,
                operand_or_throw(insn, 0).make_access(),
                std::stoull(
                    operand_or_throw(insn, 1).ingredients.front().text)}
                .encode();
    case FORMAT::R:
    {
        auto const imm = insn.operands.back().ingredients.front();
        auto const is_unsigned = (static_cast<opcode_type>(opcode)
                                  & viua::arch::ops::UNSIGNED);
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
            return
                viua::arch::ops::R{
                    opcode,
                    insn.operands.at(0).make_access(),
                    insn.operands.at(1).make_access(),
                    (is_unsigned
                         ? static_cast<uint32_t>(std::stoul(imm.text))
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
    default:
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{insn.opcode, Cause::Unknown_opcode, "cannot emit instruction"};
    }
}
}  // namespace viua::libs::stage
