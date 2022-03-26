/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#include <assert.h>

#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <string_view>
#include <vector>

#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/libs/stage.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>


namespace viua::libs::lexer {
auto Lexeme::operator==(TOKEN const tk) const -> bool
{
    return (token == tk);
}
auto Lexeme::operator==(std::string_view const sv) const -> bool
{
    return (text == sv);
}
auto Lexeme::make_synth(std::string sv, TOKEN const tk) const -> Lexeme
{
    auto sy             = *this;
    sy.text             = std::move(sv);
    sy.token            = tk;
    sy.synthesized_from = {text, token, location};
    return sy;
}
auto Lexeme::is_synth() const -> bool
{
    return synthesized_from.has_value();
}
auto Lexeme::synthed_from() const -> Lexeme
{
    auto l     = Lexeme{};
    l.text     = std::get<0>(*synthesized_from);
    l.token    = std::get<1>(*synthesized_from);
    l.location = std::get<2>(*synthesized_from);
    return l;
}

auto to_string(TOKEN const token) -> std::string
{
    switch (token) {
    case TOKEN::IMPORT:
        return "IMPORT";
    case TOKEN::EXPORT:
        return "EXPORT";
    case TOKEN::OPCODE:
        return "OPCODE";
    case TOKEN::COMMA:
        return "COMMA";
    case TOKEN::DOT:
        return "DOT";
    case TOKEN::EQ:
        return "EQ";
    case TOKEN::AT:
        return "AT";
    case TOKEN::TERMINATOR:
        return "TERMINATOR";
    case TOKEN::ATTR_LIST_OPEN:
        return "ATTR_LIST_OPEN";
    case TOKEN::ATTR_LIST_CLOSE:
        return "ATTR_LIST_CLOSE";
    case TOKEN::PAREN_OPEN:
        return "PAREN_OPEN";
    case TOKEN::PAREN_CLOSE:
        return "PAREN_CLOSE";
    case TOKEN::BRACE_OPEN:
        return "BRACE_OPEN";
    case TOKEN::BRACE_CLOSE:
        return "BRACE_CLOSE";
    case TOKEN::RA_VOID:
        return "RA_VOID";
    case TOKEN::RA_DIRECT:
        return "RA_DIRECT";
    case TOKEN::RA_PTR_DEREF:
        return "RA_PTR_DEREF";
    case TOKEN::LITERAL_STRING:
        return "LITERAL_STRING";
    case TOKEN::LITERAL_INTEGER:
        return "LITERAL_INTEGER";
    case TOKEN::LITERAL_FLOAT:
        return "LITERAL_FLOAT";
    case TOKEN::LITERAL_ATOM:
        return "LITERAL_ATOM";
    case TOKEN::DEF_FUNCTION:
        return "DEF_FUNCTION";
    case TOKEN::DEF_BLOCK:
        return "DEF_BLOCK";
    case TOKEN::DEF_LABEL:
        return "DEF_LABEL";
    case TOKEN::DEF_VALUE:
        return "DEF_VALUE";
    case TOKEN::END:
        return "END";
    case TOKEN::WHITESPACE:
        return "WHITESPACE";
    case TOKEN::COMMENT:
        return "COMMENT";
    }

    // impossible, because all cases are handled in the enumeration above
    assert(0);
}

const auto COMMENT = std::regex{"^[;#].*"};

const auto DEF_FUNCTION = std::regex{"^.function:"};
const auto END          = std::regex{"^.end"};
const auto DEF_LABEL    = std::regex{"^.label:"};
const auto DEF_VALUE    = std::regex{"^.value:"};

const auto WHITESPACE = std::regex{"^[ \t]+"};

const auto RA_DIRECT    = std::regex{"^\\$"};
const auto RA_PTR_DEREF = std::regex{"^\\*"};
const auto RA_VOID      = std::regex{"^\\bvoid\\b"};

const auto OPCODE = std::regex{"^(?:g.)?[a-z][a-z0-9_]*(?:.[stw])?\\b"};

const auto COMMA           = std::regex{"^,"};
const auto DOT             = std::regex{"^\\."};
const auto EQ              = std::regex{"^="};
const auto AT              = std::regex{"^@"};
const auto ATTR_LIST_OPEN  = std::regex{"^\\[\\["};
const auto ATTR_LIST_CLOSE = std::regex{"^\\]\\]"};

const auto LITERAL_ATOM = std::regex{"^[A-Za-z_][A-Za-z0-9:_/()<>]+\\b"};
const auto LITERAL_INTEGER =
    std::regex{"^-?(?:0x[a-f0-9]+|0o[0-7]+|0b[01]+|0|[1-9][0-9]*)u?\\b"};
const auto LITERAL_FLOAT = std::regex{"^-?(?:0|[1-9][0-9]*)?\\.[0-9]+\\b"};

auto lex(std::string_view source_text) -> std::vector<Lexeme>
{
    auto lexemes = std::vector<Lexeme>{};

    auto line      = size_t{};
    auto character = size_t{};
    auto offset    = size_t{};

    while (not source_text.empty()) {
        if (source_text[0] == '\n') {
            lexemes.emplace_back(
                "\n", TOKEN::TERMINATOR, Location{line, character, offset});

            line += 1;
            character = 0;
            offset += 1;
            source_text.remove_prefix(1);

            continue;
        }
        if (source_text[0] == '\\') {
            if (source_text.size() == 1 or source_text[1] != '\n') {
                throw Location{line, character, offset};  // invalid line
                                                          // continuation
            }

            line += 1;
            character = 0;
            offset += 1;
            source_text.remove_prefix(2);

            continue;
        }
        if (source_text[0] == '"') {
            if constexpr (false) {
                auto ss = std::stringstream{};
                ss << std::string_view{
                    source_text.begin(),
                    std::find(source_text.begin(), source_text.end(), '\n')};

                /*
                 * Use a null byte as an "escape character" because it should
                 * not appear in a string - it will be encoded as \x00 (with
                 * four characters), \0 (with two characters) or something else.
                 * The important thing is that it will not appear as ITSELF.
                 *
                 * If we used the \ ie, the backslash character all hell would
                 * break loose and the code would stop working because
                 * std::quoted() would remove it, even if it appeared as a part
                 * of an escape sequence - for example \n.
                 *
                 * Why, though? I think this is clearly a broken behaviour. If
                 * the std::quoted() does not escape characters other than "
                 * then why would it "unescape" them?
                 */
                auto extracted_string = std::string{};
                ss >> std::quoted(extracted_string, '"', '\x00');

                /*
                 * We need to add 2 to the amount to shift the head of
                 * std::string_view by, because std::quoted() does not extract
                 * the quotes.
                 */
                auto const shift_by = (extracted_string.size() + 2);

                lexemes.emplace_back(std::move(extracted_string),
                                     TOKEN::LITERAL_STRING,
                                     Location{line, character, offset});

                character += shift_by;
                offset += shift_by;
                source_text.remove_prefix(shift_by);
            }

            auto ss                  = std::ostringstream{};
            auto escaped             = bool{false};
            constexpr auto DELIMITER = '"';

            ss << source_text.front();
            for (auto i = size_t{1}; i < source_text.size(); ++i) {
                auto const c = source_text[i];
                ss << c;

                if (c == DELIMITER and not escaped) {
                    break;
                }

                escaped = (c == '\\') ? (not escaped) : false;
            }

            auto extracted_string = ss.str();
            auto const shift_by   = extracted_string.size();

            lexemes.emplace_back(std::move(extracted_string),
                                 TOKEN::LITERAL_STRING,
                                 Location{line, character, offset});

            character += shift_by;
            offset += shift_by;
            source_text.remove_prefix(shift_by);

            continue;
        }

        auto const try_match =
            [&lexemes, &source_text, &line, &character, &offset](
                std::regex const& re, TOKEN const tt) -> bool {
            std::cmatch m;
            if (regex_search(source_text.data(), m, re)) {
                lexemes.emplace_back(
                    m.str(), tt, Location{line, character, offset});

                character += lexemes.back().text.size();
                offset += lexemes.back().text.size();
                source_text.remove_prefix(lexemes.back().text.size());
            }
            return (not m.empty());
        };

        if (try_match(WHITESPACE, TOKEN::WHITESPACE)) {
            continue;
        }
        if (try_match(COMMENT, TOKEN::COMMENT)) {
            continue;
        }
        if (try_match(DEF_FUNCTION, TOKEN::DEF_FUNCTION)) {
            continue;
        }
        if (try_match(END, TOKEN::END)) {
            continue;
        }
        if (try_match(DEF_LABEL, TOKEN::DEF_LABEL)) {
            continue;
        }
        if (try_match(DEF_VALUE, TOKEN::DEF_VALUE)) {
            continue;
        }
        if (try_match(COMMA, TOKEN::COMMA)) {
            continue;
        }
        if (try_match(DOT, TOKEN::DOT)) {
            continue;
        }
        if (try_match(RA_VOID, TOKEN::RA_VOID)) {
            continue;
        }
        if (try_match(RA_DIRECT, TOKEN::RA_DIRECT)) {
            continue;
        }
        if (try_match(RA_PTR_DEREF, TOKEN::RA_PTR_DEREF)) {
            continue;
        }
        if (try_match(OPCODE, TOKEN::OPCODE)) {
            if (OPCODE_NAMES.count(lexemes.back().text) == 0) {
                auto lx = std::move(lexemes.back());
                lexemes.pop_back();

                lexemes.emplace_back(lx.text, TOKEN::LITERAL_ATOM, lx.location);
            }
            continue;
        }
        if (try_match(LITERAL_ATOM, TOKEN::LITERAL_ATOM)) {
            continue;
        }
        if (try_match(LITERAL_FLOAT, TOKEN::LITERAL_FLOAT)) {
            try {
                std::stod(lexemes.back().text);
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{lexemes.back(), Cause::Value_out_of_range};
            }
            continue;
        }
        if (try_match(LITERAL_INTEGER, TOKEN::LITERAL_INTEGER)) {
            try {
                auto const sv = std::string_view{lexemes.back().text};
                if (sv.starts_with("0x")) {
                    std::stoull(lexemes.back().text, nullptr, 16);
                } else if (sv.starts_with("0o")) {
                    std::stoull(lexemes.back().text, nullptr, 8);
                } else if (sv.starts_with("0b")) {
                    std::stoull(lexemes.back().text, nullptr, 2);
                } else {
                    std::stoull(lexemes.back().text, nullptr);
                }
            } catch (std::out_of_range const&) {
                auto bits_needed = size_t{0};
                auto const sv    = std::string_view{lexemes.back().text};
                if (sv.starts_with("0x")) {
                    bits_needed = ((sv.size() - 2) * 4);
                } else if (sv.starts_with("0o")) {
                    bits_needed = ((sv.size() - 2) * 3);
                } else if (sv.starts_with("0b")) {
                    bits_needed = (sv.size() - 2);
                } else {
                    // FIXME number of bits for a decimal integer
                }

                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{lexemes.back(), Cause::Value_out_of_range}.aside(
                    "register width is 64 bits"
                    + (bits_needed ? (", but this value needs "
                                      + std::to_string(bits_needed))
                                   : ""));
            }
            continue;
        }
        if (try_match(ATTR_LIST_OPEN, TOKEN::ATTR_LIST_OPEN)) {
            continue;
        }
        if (try_match(ATTR_LIST_CLOSE, TOKEN::ATTR_LIST_CLOSE)) {
            continue;
        }
        if (try_match(EQ, TOKEN::EQ)) {
            continue;
        }
        if (try_match(AT, TOKEN::AT)) {
            continue;
        }

        throw Location{line, character, offset};
    }

    return lexemes;
}

namespace stage {
auto lexical_analysis(std::filesystem::path const source_path,
                      std::string_view const source_text) -> std::vector<Lexeme>
{
    try {
        return viua::libs::lexer::lex(source_text);
    } catch (viua::libs::lexer::Location const& location) {
        using viua::support::tty::ATTR_RESET;
        using viua::support::tty::COLOR_FG_ORANGE_RED_1;
        using viua::support::tty::COLOR_FG_RED;
        using viua::support::tty::COLOR_FG_RED_1;
        using viua::support::tty::COLOR_FG_WHITE;
        using viua::support::tty::send_escape_seq;
        constexpr auto esc = send_escape_seq;

        auto const SEPARATOR         = std::string{" |  "};
        constexpr auto LINE_NO_WIDTH = size_t{5};

        auto source_line    = std::ostringstream{};
        auto highlight_line = std::ostringstream{};

        std::cerr << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR << "\n";

        struct {
            std::string text;
        } e;
        e.text = std::string(1, source_text[location.offset]);

        {
            auto line = viua::libs::stage::view_line_of(source_text, location);

            source_line << esc(2, COLOR_FG_RED) << std::setw(LINE_NO_WIDTH)
                        << (location.line + 1) << esc(2, ATTR_RESET)
                        << SEPARATOR;
            highlight_line << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR;

            source_line << std::string_view{line.data(), location.character};
            highlight_line << std::string(location.character, ' ');
            line.remove_prefix(location.character);

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
                source_line << esc(2, COLOR_FG_RED_1) << e.text
                            << esc(2, ATTR_RESET);
                line.remove_prefix(e.text.size());
            }
            highlight_line << esc(2, COLOR_FG_RED) << '^';
            highlight_line << esc(2, COLOR_FG_ORANGE_RED_1)
                           << std::string((e.text.size() - 1), '~');

            source_line << line;
        }

        std::cerr << source_line.str() << "\n";
        std::cerr << highlight_line.str() << "\n";

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (location.line + 1) << esc(2, ATTR_RESET) << ':'
                  << esc(2, COLOR_FG_WHITE) << (location.character + 1)
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET) << ": "
                  << "no token match at character "
                  << viua::support::string::CORNER_QUOTE_LL
                  << esc(2, COLOR_FG_WHITE) << e.text << esc(2, ATTR_RESET)
                  << viua::support::string::CORNER_QUOTE_UR << "\n";

        exit(1);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
}
}
}  // namespace viua::libs::lexer
