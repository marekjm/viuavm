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

#include <iostream>
#include <regex>
#include <set>
#include <string_view>
#include <vector>

#include <ctre/single-header/ctre-unicode.hpp>

#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/libs/stage.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>

namespace viua::libs::lexer {
auto Lexeme::operator==(
    TOKEN const tk) const -> bool
{
    return (token == tk);
}
auto Lexeme::operator==(
    std::string_view const sv) const -> bool
{
    return (text == sv);
}
auto Lexeme::make_synth(
    std::string sv,
    TOKEN const tk) const -> Lexeme
{
    auto sy             = *this;
    sy.text             = std::move(sv);
    sy.token            = tk;
    sy.synthesized_from = { text, token, location };
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

auto to_string(
    TOKEN const token) -> std::string
{
    switch (token) {
        using enum TOKEN;
        case INVALID:
            return "INVALID";
        case WHITESPACE:
            return "WHITESPACE";
        case COMMENT:
            return "COMMENT";
        case SWITCH_TO_TEXT:
            return "SWITCH_TO_TEXT";
        case SWITCH_TO_RODATA:
            return "SWITCH_TO_RODATA";
        case SWITCH_TO_SECTION:
            return "SWITCH_TO_SECTION";
        case DECLARE_SYMBOL:
            return "DECLARE_SYMBOL";
        case DEFINE_LABEL:
            return "DEFINE_LABEL";
        case BEGIN:
            return "BEGIN";
        case END:
            return "END";
        case ALLOCATE_OBJECT:
            return "ALLOCATE_OBJECT";
        case OPCODE:
            return "OPCODE";
        case VOID:
            return "VOID";
        case ZERO_SIGNED:
            return "ZERO_SIGNED";
        case ZERO_UNSIGNED:
            return "ZERO_UNSIGNED";
        case LITERAL_ATOM:
            return "LITERAL_ATOM";
        case LITERAL_INTEGER:
            return "LITERAL_INTEGER";
        case LITERAL_FLOAT:
            return "LITERAL_FLOAT";
        case LITERAL_STRING:
            return "LITERAL_STRING";
        case COMMA:
            return "COMMA";
        case ELLIPSIS:
            return "ELLIPSIS";
        case DOT:
            return "DOT";
        case EQ:
            return "EQ";
        case AT:
            return "AT";
        case DOLLAR:
            return "DOLLAR";
        case STAR:
            return "STAR";
        case TERMINATOR:
            return "TERMINATOR";
        case ATTR_LIST_OPEN:
            return "ATTR_LIST_OPEN";
        case ATTR_LIST_CLOSE:
            return "ATTR_LIST_CLOSE";
    }

    // impossible, because all cases are handled in the enumeration above
    assert(0);
}

const auto WHITESPACE = ctre::starts_with<"^[ \t]+">;

const auto COMMENT = ctre::multiline_starts_with<"[;#].*">;

const auto DIR_TEXT    = ctre::starts_with<"\\.text\\b">;
const auto DIR_RODATA  = ctre::starts_with<"\\.rodata\\b">;
const auto DIR_SECTION = ctre::starts_with<"\\.section\\b">;

const auto DIR_SYMBOL = ctre::starts_with<"\\.symbol\\b">;
const auto DIR_LABEL  = ctre::starts_with<"\\.label\\b">;
const auto DIR_BEGIN  = ctre::starts_with<"\\.begin\\b">;
const auto DIR_END    = ctre::starts_with<"\\.end\\b">;
const auto DIR_OBJECT = ctre::starts_with<"\\.object\\b">;

/*
 * The regex for register indexes will catch ANYTHING up until word boundary,
 * not only valid index names. This is a better choice for error reporting since
 * it allows the assembler to quickly see what's wrong; if the regex only
 * accepted valid names the errors could get weird, and it would be much more
 * difficult to provide sane diagnostics.
 */
const auto VOID          = ctre::starts_with<"void\\b">;
const auto ZERO_SIGNED   = ctre::starts_with<"zero\\b">;
const auto ZERO_UNSIGNED = ctre::starts_with<"uzero\\b">;

const auto LITERAL_ATOM = ctre::starts_with<pattern::LITERAL_ATOM>;
const auto LITERAL_INTEGER_HEX =
    ctre::starts_with<"-?0x[0-9a-f]('?[0-9a-f]+)*u?">;
const auto LITERAL_INTEGER_OCT = ctre::starts_with<"-?0o[0-7]('?[0-7]+)*u?">;
const auto LITERAL_INTEGER_BIN = ctre::starts_with<"-?0b[01]('?[01]+)*u?">;
const auto LITERAL_INTEGER_DEC =
    ctre::starts_with<"-?(?:0|[1-9](?:'?[0-9]+)*)u?">;
const auto LITERAL_FLOAT = ctre::starts_with<"-?(?:0|[1-9][0-9]*)?\\.[0-9]+">;

const auto COMMA           = ctre::starts_with<",">;
const auto ELLIPSIS        = ctre::starts_with<"\\.\\.\\.">;
const auto DOT             = ctre::starts_with<"\\.">;
const auto EQ              = ctre::starts_with<"=">;
const auto AT              = ctre::starts_with<"@">;
const auto DOLLAR          = ctre::starts_with<"\\$">;
const auto STAR            = ctre::starts_with<"\\*">;
const auto ATTR_LIST_OPEN  = ctre::starts_with<"\\[\\[">;
const auto ATTR_LIST_CLOSE = ctre::starts_with<"\\]\\]">;

const auto OPCODE =
    ctre::starts_with<"[a-z_]+(?:\\.(?:native|saturate|wrap|trap))?\\b">;

namespace {
auto match_lookbehind [[maybe_unused]] (
    std::vector<Lexeme> const& lexemes,
    std::vector<TOKEN> const pattern) -> bool
{
    if (pattern.empty()) {
        return true;
    }
    if (pattern.size() > lexemes.size()) {
        return false;
    }

    using size_type = decltype(pattern)::size_type;
    for (auto i = size_type{ 0 }; i < pattern.size(); ++i) {
        auto const pi = pattern.size() - 1 - i;
        auto const li = lexemes.size() - 1 - i;
        if (pattern.at(pi) != lexemes.at(li).token) {
            return false;
        }
    }

    return true;
}

auto synth_lookbehind [[maybe_unused]] (
    std::vector<Lexeme> const& lexemes,
    size_t const n) -> Lexeme
{
    if (n > lexemes.size()) {
        std::cerr
            << "internal error: cannot synthesize lexeme from lookbehind\n";
        assert(false);
    }

    auto const location = lexemes.at(lexemes.size() - n).location;
    auto const token    = lexemes.at(lexemes.size() - n).token;

    auto text = std::string{};
    for (auto i = size_t{ 0 }; i < n; ++i) {
        text += lexemes.at(lexemes.size() - n + i).text;
    }

    return Lexeme{ text, token, location };
}
}  // namespace

auto lex(
    std::string_view source_text) -> std::vector<Lexeme>
{
    auto lexemes = std::vector<Lexeme>{};

    auto line      = size_t{};
    auto character = size_t{};
    auto offset    = size_t{};

    auto const consume = [&lexemes, &source_text, &line, &character, &offset](
                             std::string&& v, TOKEN const t) -> void
    {
        lexemes.emplace_back(
            std::move(v), t, Location{ line, character, offset });


        auto const consumed_chars = lexemes.back().text.size();
        character += consumed_chars;
        offset += consumed_chars;
        source_text.remove_prefix(consumed_chars);
    };

    while (not source_text.empty()) {
        if (source_text[0] == '\n') {
            lexemes.emplace_back(
                "\n", TOKEN::TERMINATOR, Location{ line, character, offset });

            line += 1;
            character = 0;
            offset += 1;
            source_text.remove_prefix(1);

            continue;
        }
        if (source_text[0] == '\\') {
            if (source_text.size() == 1 or source_text[1] != '\n') {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto const lexeme =
                    Lexeme{ std::string{ 1, source_text[0] },
                            TOKEN::INVALID,
                            Location{ line, character, offset } };

                throw Error{ lexeme, Cause::Unexpected_token }.aside(
                    "line continuation not immediately followed by \\n "
                    "character");
            }

            line += 1;
            character = 0;
            offset += 2;
            source_text.remove_prefix(2);

            continue;
        }
        if (source_text[0] == '"') {
            if constexpr (false) {
                auto ss = std::stringstream{};
                ss << std::string_view{
                    source_text.begin(),
                    std::find(source_text.begin(), source_text.end(), '\n')
                };

                /*
                 * Use a null byte as an "escape character" because it should
                 * not appear in a string - it will be encoded as \x00 (with
                 * four characters), \0 (with two characters) or something else.
                 * The important thing is that it will not appear as ITSELF.
                 *
                 * If we used the \ ie, the backslash character, all hell would
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

                lexemes.emplace_back(
                    Lexeme{ std::move(extracted_string),
                            TOKEN::LITERAL_STRING,
                            Location{ line, character, offset } });

                character += shift_by;
                offset += shift_by;
                source_text.remove_prefix(shift_by);
            }

            auto ss                  = std::ostringstream{};
            auto escaped             = bool{ false };
            constexpr auto DELIMITER = '"';

            ss << source_text.front();
            for (auto i = size_t{ 1 }; i < source_text.size(); ++i) {
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
                                 Location{ line, character, offset });

            character += shift_by;
            offset += shift_by;
            source_text.remove_prefix(shift_by);

            continue;
        }

        if (auto const m = WHITESPACE(source_text)) {
            consume(m.to_string(), TOKEN::WHITESPACE);
            continue;
        }
        if (auto const m = COMMENT(source_text)) {
            consume(m.to_string(), TOKEN::COMMENT);
            continue;
        }
        if (auto const m = DIR_TEXT(source_text)) {
            consume(m.to_string(), TOKEN::SWITCH_TO_TEXT);
            continue;
        }
        if (auto const m = DIR_RODATA(source_text)) {
            consume(m.to_string(), TOKEN::SWITCH_TO_RODATA);
            continue;
        }
        if (auto const m = DIR_SECTION(source_text)) {
            consume(m.to_string(), TOKEN::SWITCH_TO_SECTION);
            continue;
        }
        if (auto const m = DIR_SYMBOL(source_text)) {
            consume(m.to_string(), TOKEN::DECLARE_SYMBOL);
            continue;
        }
        if (auto const m = DIR_LABEL(source_text)) {
            consume(m.to_string(), TOKEN::DEFINE_LABEL);
            continue;
        }
        if (auto const m = DIR_BEGIN(source_text)) {
            consume(m.to_string(), TOKEN::BEGIN);
            continue;
        }
        if (auto const m = DIR_END(source_text)) {
            consume(m.to_string(), TOKEN::END);
            continue;
        }
        if (auto const m = DIR_OBJECT(source_text)) {
            consume(m.to_string(), TOKEN::ALLOCATE_OBJECT);
            continue;
        }

        if (auto const m = LITERAL_FLOAT(source_text)) {
            consume(m.to_string(), TOKEN::LITERAL_FLOAT);
            continue;
        }
        if (auto const m = LITERAL_INTEGER_HEX(source_text)) {
            consume(m.to_string(), TOKEN::LITERAL_INTEGER);
            continue;
        }
        if (auto const m = LITERAL_INTEGER_OCT(source_text)) {
            consume(m.to_string(), TOKEN::LITERAL_INTEGER);
            continue;
        }
        if (auto const m = LITERAL_INTEGER_BIN(source_text)) {
            consume(m.to_string(), TOKEN::LITERAL_INTEGER);
            continue;
        }
        if (auto const m = LITERAL_INTEGER_DEC(source_text)) {
            consume(m.to_string(), TOKEN::LITERAL_INTEGER);
            continue;
        }

        if (auto const m = VOID(source_text)) {
            consume(m.to_string(), TOKEN::VOID);
            continue;
        }

        if (auto const m = ZERO_SIGNED(source_text)) {
            consume(m.to_string(), TOKEN::ZERO_SIGNED);
            continue;
        }
        if (auto const m = ZERO_UNSIGNED(source_text)) {
            consume(m.to_string(), TOKEN::ZERO_UNSIGNED);
            continue;
        }

        if (auto const m = OPCODE(source_text)) {
            consume(m.to_string(), TOKEN::OPCODE);
            auto const not_really_an_opcode =
                not OPCODE_NAMES.contains(lexemes.back().text);
            auto const looks_atomish = [&lexemes]() -> bool
            {
                return ctre::match<pattern::LITERAL_ATOM>(lexemes.back().text);
            }();
            if (not_really_an_opcode and looks_atomish) {
                auto lx = std::move(lexemes.back());
                lexemes.pop_back();

                lexemes.emplace_back(lx.text, TOKEN::LITERAL_ATOM, lx.location);
            }
            continue;
        }
        if (auto const m = LITERAL_ATOM(source_text)) {
            consume(m.to_string(), TOKEN::LITERAL_ATOM);
            continue;
        }

        if (auto const m = ATTR_LIST_OPEN(source_text)) {
            consume(m.to_string(), TOKEN::ATTR_LIST_OPEN);
            continue;
        }
        if (auto const m = ATTR_LIST_CLOSE(source_text)) {
            consume(m.to_string(), TOKEN::ATTR_LIST_CLOSE);
            continue;
        }

        if (auto const m = COMMA(source_text)) {
            consume(m.to_string(), TOKEN::COMMA);
            continue;
        }
        if (auto const m = ELLIPSIS(source_text)) {
            consume(m.to_string(), TOKEN::ELLIPSIS);
            continue;
        }
        if (auto const m = EQ(source_text)) {
            consume(m.to_string(), TOKEN::EQ);
            continue;
        }
        if (auto const m = DOT(source_text)) {
            consume(m.to_string(), TOKEN::DOT);
            continue;
        }
        if (auto const m = DOLLAR(source_text)) {
            consume(m.to_string(), TOKEN::DOLLAR);
            continue;
        }
        if (auto const m = STAR(source_text)) {
            consume(m.to_string(), TOKEN::STAR);
            continue;
        }
        if (auto const m = AT(source_text)) {
            consume(m.to_string(), TOKEN::AT);
            continue;
        }

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;

        auto const lexeme = Lexeme{ std::string(1, source_text[0]),
                                    TOKEN::INVALID,
                                    Location{ line, character, offset } };

        throw Error{ lexeme, Cause::Illegal_character };
    }

    return lexemes;
}

namespace stage {
auto lex(
    std::filesystem::path const source_path,
    std::string_view const source_text) -> std::vector<Lexeme>
{
    try {
        return viua::libs::lexer::lex(source_text);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
}

auto remove_noise(
    std::vector<Lexeme>&& raw) -> std::vector<Lexeme>
{
    using viua::libs::lexer::TOKEN;

    auto tmp = std::vector<viua::libs::lexer::Lexeme>{};
    for (auto& each : raw) {
        if (each.token == TOKEN::WHITESPACE or each.token == TOKEN::COMMENT) {
            continue;
        }

        tmp.push_back(std::move(each));
    }

    while ((not tmp.empty()) and tmp.front().token == TOKEN::TERMINATOR) {
        tmp.erase(tmp.begin());
    }

    auto cooked = std::vector<viua::libs::lexer::Lexeme>{};
    for (auto& each : tmp) {
        if (each.token != TOKEN::TERMINATOR or cooked.empty()) {
            cooked.push_back(std::move(each));
            continue;
        }

        if (cooked.back().token == TOKEN::TERMINATOR) {
            continue;
        }

        cooked.push_back(std::move(each));
    }

    return cooked;
}
}  // namespace stage
}  // namespace viua::libs::lexer
