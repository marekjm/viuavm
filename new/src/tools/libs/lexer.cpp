/*
 *  Copyright (C) 2021 Marek Marecki
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

#include <viua/libs/lexer.h>
#include <viua/support/string.h>

#include <iomanip>
#include <iostream>
#include <regex>
#include <string_view>
#include <vector>

#include <assert.h>


namespace viua::libs::lexer {
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
        case TOKEN::TERMINATOR:
            return "TERMINATOR";
        case TOKEN::ATTR_OPEN:
            return "ATTR_OPEN";
        case TOKEN::ATTR_CLOSE:
            return "ATTR_CLOSE";
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
    const auto END = std::regex{"^.end"};

    const auto WHITESPACE = std::regex{"^[ \t]+"};

    const auto RA_VOID = std::regex{"^\\bvoid\\b"};

    const auto OPCODE = std::regex{"^(?:g.)?[a-z][a-z0-9_]*(?:.[stw])?\\b"};

    const auto LITERAL_ATOM = std::regex{"^[A-Za-z_][A-Za-z0-9:_/()<>]+\\b"};
    const auto LITERAL_INTEGER = std::regex{"^-?(?:0x[a-f0-9]+|0o[0-7]+|0b[01]+|0|[1-9][0-9]*)\\b"};

    auto lex(std::string_view source_text) -> std::vector<Lexeme>
    {
        auto lexemes = std::vector<Lexeme>{};

        auto line = size_t{};
        auto character = size_t{};
        auto offset = size_t{};
        while (not source_text.empty()) {
            if (source_text[0] == '\n') {
                lexemes.emplace_back("\n", TOKEN::TERMINATOR, Location{ line, character, offset });

                line += 1;
                character = 0;
                offset += 1;
                source_text.remove_prefix(1);

                continue;
            }
            if (source_text[0] == '\\') {
                if (source_text.size() == 1 or source_text[1] != '\n') {
                    throw Location{line, character, offset}; // invalid line continuation
                }

                line += 1;
                character = 0;
                offset += 1;
                source_text.remove_prefix(2);

                continue;
            }
            if (source_text[0] == '$') {
                lexemes.emplace_back("$", TOKEN::RA_DIRECT, Location{ line, character, offset });

                character += 1;
                offset += 1;
                source_text.remove_prefix(1);

                continue;
            }
            if (source_text[0] == '*') {
                lexemes.emplace_back("*", TOKEN::RA_DIRECT, Location{ line, character, offset });

                character += 1;
                offset += 1;
                source_text.remove_prefix(1);

                continue;
            }
            if (source_text[0] == ',') {
                lexemes.emplace_back(",", TOKEN::COMMA, Location{ line, character, offset });

                character += 1;
                offset += 1;
                source_text.remove_prefix(1);

                continue;
            }

            if (source_text[0] == '"') {
                auto ss = std::stringstream{};
                ss << std::string_view{
                      source_text.begin()
                    , std::find(source_text.begin(), source_text.end(), '\n')
                };

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
                 * We need to add 2 to the amount to shift the head of std::string_view
                 * by, because std::quoted() does not extract the quotes.
                 */
                auto const shift_by = (extracted_string.size() + 2);

                character += shift_by;
                offset += shift_by;
                source_text.remove_prefix(shift_by);

                lexemes.emplace_back(
                      std::move(extracted_string)
                    , TOKEN::COMMA
                    , Location{ line, character, offset }
                );

                continue;
            }

            auto const try_match =
                [&lexemes, &source_text, &line, &character, &offset](std::regex const& re, TOKEN const tt)
                    -> bool
            {
                std::cmatch m;
                if (regex_search(source_text.data(), m, re)) {
                    lexemes.emplace_back(m.str(), tt, Location{ line, character, offset });

                    character += lexemes.back().text.size();
                    offset += lexemes.back().text.size();
                    source_text.remove_prefix(lexemes.back().text.size());
                }
                return (not m.empty());
            };

            if (try_match(WHITESPACE, TOKEN::WHITESPACE)) { continue; }
            if (try_match(COMMENT, TOKEN::COMMENT)) { continue; }
            if (try_match(RA_VOID, TOKEN::RA_VOID)) { continue; }
            if (try_match(OPCODE, TOKEN::OPCODE)) { continue; }
            if (try_match(LITERAL_ATOM, TOKEN::LITERAL_ATOM)) { continue; }
            if (try_match(LITERAL_INTEGER, TOKEN::LITERAL_INTEGER)) { continue; }
            if (try_match(DEF_FUNCTION, TOKEN::DEF_FUNCTION)) { continue; }
            if (try_match(END, TOKEN::END)) { continue; }

            std::cerr << viua::support::string::quoted(source_text.substr(0, 40)) << "\n";
            throw Location{line, character, offset};
        }

        return lexemes;
    }
}
