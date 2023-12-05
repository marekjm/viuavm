/*
 *  Copyright (C) 2021-2023 Marek Marecki
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
#include <elf.h>
#include <endian.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <experimental/memory>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/elf.h>
#include <viua/arch/ops.h>
#include <viua/libs/assembler.h>
#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/libs/stage.h>
#include <viua/support/errno.h>
#include <viua/support/fdio.h>
#include <viua/support/number.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/support/vector.h>


constexpr auto DEBUG_LEX                        = true;
constexpr auto DEBUG_PARSE                      = true;
constexpr auto DEBUG_EXPANSION [[maybe_unused]] = false;

using viua::libs::stage::save_buffer_to_rodata;
using viua::libs::stage::save_string_to_strtab;
using viua::support::string::quote_fancy;

namespace {
auto const FUNDAMENTAL_TYPE_NAMES = std::map<std::string, uint8_t>{
    {"int", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::INT)},
    {"uint", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::UINT)},
    {"float", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::FLOAT32)},
    {"double", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::FLOAT64)},
    {"pointer", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::POINTER)},
    {"atom", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::ATOM)},
    {"pid", static_cast<uint8_t>(viua::arch::FUNDAMENTAL_TYPES::PID)},
};
}

namespace {
using viua::libs::lexer::Lexeme;
auto dump_lexemes(std::vector<Lexeme> const& lexemes) -> void
{
    if (lexemes.empty()) {
        return;
    }

    auto const max_line = lexemes.back().location.line;
    auto const pad_line = std::to_string(max_line).size();

    auto const max_char =
        std::max_element(lexemes.begin(),
                         lexemes.end(),
                         [](Lexeme const& lhs, Lexeme const& rhs) -> bool {
                             return lhs.location.character
                                    < rhs.location.character;
                         })
            ->location.character;
    auto const max_size =
        std::max_element(lexemes.begin(),
                         lexemes.end(),
                         [](Lexeme const& lhs, Lexeme const& rhs) -> bool {
                             return lhs.text.size() < rhs.text.size();
                         })
            ->text.size();
    auto const pad_location =
        1 + std::to_string(max_char).size() + std::to_string(max_size).size();

    auto const max_offset = lexemes.back().location.offset;
    auto const pad_offset = std::to_string(max_offset).size();

    for (auto const& each : lexemes) {
        auto token_kind = viua::libs::lexer::to_string(each.token);
        token_kind.resize(17, ' ');

        auto loc =
            std::to_string(each.location.character) + '-'
            + std::to_string(each.location.character + each.text.size() - 1);
        loc.resize(pad_location, ' ');

        auto off = "+" + std::to_string(each.location.offset);
        off.resize(2 + pad_offset, ' ');

        std::cerr << "  " << token_kind;
        std::cerr << "  " << std::setw(pad_line) << each.location.line << ':'
                  << loc;
        std::cerr << "  " << off;

        using viua::libs::lexer::TOKEN;
        auto const printable = (each.token == TOKEN::LITERAL_STRING)
                               or (each.token == TOKEN::LITERAL_INTEGER)
                               or (each.token == TOKEN::LITERAL_FLOAT)
                               or (each.token == TOKEN::LITERAL_ATOM)
                               or (each.token == TOKEN::OPCODE);
        if (printable) {
            std::cerr << "  " << each.text;
        }

        std::cerr << "\n";
    }
}

auto looks_octal(std::string_view const sv) -> bool
{
    return std::all_of(sv.begin(), sv.end(), [](char const c) -> bool {
        return (c >= '0') and (c <= '7');
    });
}
auto any_find_mistake(std::vector<Lexeme> const& lexemes)
    -> std::optional<viua::libs::errors::compile_time::Error>
{
    if (lexemes.empty()) {
        return std::nullopt;
    }

    using viua::libs::lexer::TOKEN;

    auto const at = [&lexemes](size_t const n) -> Lexeme const& {
        return lexemes.at(n);
    };
    auto const is = [&lexemes](size_t const n, TOKEN const t) -> bool {
        return lexemes.at(n).token == t;
    };
    auto const iso =
        [&lexemes](size_t const n, ssize_t const off, TOKEN const t) -> bool {
        if ((off < 0) and (static_cast<size_t>(std::abs(off)) > n)) {
            return false;
        }
        auto const effective_n = n + off;
        return lexemes.at(effective_n).token == t;
    };
    auto const next_is_glued = [&lexemes](size_t const n) -> bool {
        return (lexemes.at(n).location.offset + lexemes.at(n).text.size())
               == lexemes.at(n + 1).location.offset;
    };

    /*
     * Detect invalid numeric literals.
     */
    for (auto i = size_t{0}; i < (lexemes.size() - 1); ++i) {
        if (not next_is_glued(i)) {
            continue;
        }
        if (is(i + 1, TOKEN::TERMINATOR)) {
            continue;
        }

        constexpr auto NOT_A_VALID_NUMERIC_LITERAL =
            "not a valid numeric literal";
        if (is(i, TOKEN::LITERAL_INTEGER)
            and is(i + 1, TOKEN::LITERAL_INTEGER)) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            if (at(i).text == "0" and looks_octal(at(i + 1).text)) {
                auto e = Error{
                    at(i), Cause::Invalid_token, NOT_A_VALID_NUMERIC_LITERAL};
                e.add(at(i + 1));
                e.aside("use " + quote_fancy("0o")
                        + " prefix for octal literals");
                return e;
            }

            auto synth_text = at(i).text + at(i + 1).text;
            auto synth      = at(i).make_synth(synth_text, at(i).token);
            return Error{
                synth, Cause::Invalid_token, NOT_A_VALID_NUMERIC_LITERAL};
        }
        if (is(i, TOKEN::LITERAL_INTEGER)
            and not is(i + 1, TOKEN::LITERAL_INTEGER)) {
            /*
             * Do not raise the alarm if the numeral has a glued lexeme as part
             * of a register index. Parsing register indexes is a better place
             * to report errors in this case.
             *
             * Or if it the glued character is a comma.
             */
            auto const yeah_looks_ok = iso(i, -1, TOKEN::DOLLAR)
                                       or iso(i, +1, TOKEN::COMMA);
            if (not yeah_looks_ok) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{at(i + 1),
                               Cause::Invalid_token,
                               NOT_A_VALID_NUMERIC_LITERAL};
                e.add(at(i));
                return e;
            }
        }
    }

    return std::nullopt;
}

auto fits_in_unsigned_r_immediate(uint32_t const n) -> bool
{
    constexpr auto LOW_24 = uint32_t{0x00ffffff};
    return ((n & LOW_24) == n);
}
auto fits_in_signed_r_immediate(uint32_t const n) -> bool
{
    constexpr auto LOW_24 = uint32_t{0x00ffffff};
    auto const just_low   = (n & LOW_24);
    return (static_cast<int32_t>(n)
            == (static_cast<int32_t>(just_low << 8) >> 8));
}

auto make_name_from_lexeme(viua::libs::lexer::Lexeme const& r) -> std::string
{
    switch (r.token) {
        using enum viua::libs::lexer::TOKEN;
    case LITERAL_ATOM:
        return r.text;
    case LITERAL_STRING:
        return r.text.substr(1, r.text.size() - 2);
    default:
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{r, Cause::None, "token cannot be used as name"};
    }
}
}  // namespace

namespace {
namespace ast {
struct Node {
    using Lexeme         = viua::libs::lexer::Lexeme;
    using attribute_type = std::pair<Lexeme, std::optional<Lexeme>>;
    std::vector<attribute_type> attributes;

    Lexeme leader;

    auto has_attr(std::string_view const) const
        -> std::optional<attribute_type>;
    auto attr [[maybe_unused]] (std::string_view const) const
        -> std::optional<Lexeme>;

    virtual ~Node();
};
Node::~Node()
{}
auto Node::has_attr(std::string_view const key) const
    -> std::optional<attribute_type>
{
    for (auto const& each : attributes) {
        if (each.first == key) {
            return each;
        }
    }
    return {};
}
auto Node::attr(std::string_view const key) const -> std::optional<Lexeme>
{
    for (auto const& each : attributes) {
        if (each.first == key) {
            return each.second;
        }
    }
    return {};
}

/*
 * Node = Section String
 *      | Label Attrs? id
 *      | Symbol Attrs? id
 *      | Define_object Attrs? Type Value*
 *      | Opcode id Operands?
 *
 * Value = String
 *       | Int
 *       | Float
 *       | Atom
 *
 * Operands = Operand ("," Operand)*
 *
 * Operand = Attrs? Immediate | Register
 *
 * Attrs = "[[" Attr ("," Attr)* "]]"
 *
 * Attr = id ("=" Value)?
 */
struct Section : Node {
    Lexeme name;

    auto which() const -> std::string_view;
};
auto Section::which() const -> std::string_view
{
    return std::string_view{name.text.c_str() + 1, name.text.size() - 2};
}
struct Label : Node {
    Lexeme name;
};
struct Symbol : Node {
    Lexeme name;
};
struct Object : Node {
    Lexeme type;
    std::vector<Lexeme> ctor;
};
struct Operand : Node {
    std::vector<Lexeme> ingredients;

    auto make_access() const -> viua::arch::Register_access;
};
auto Operand::make_access() const -> viua::arch::Register_access
{
    if (ingredients.front() == "void") {
        return viua::arch::Register_access{};
    }

    using viua::libs::lexer::TOKEN;
    auto const lx = ingredients.front();
    if (lx != TOKEN::DOLLAR) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{lx, Cause::Invalid_register_access};
    }

    auto const direct = (lx == TOKEN::DOLLAR);
    auto const index  = viua::support::ston<uint8_t>(ingredients.at(1).text);
    if (ingredients.size() == 2) {
        return viua::arch::Register_access::make_local(index, direct);
    }

    auto const rs = ingredients.back();
    if (rs == "l") {
        return viua::arch::Register_access::make_local(index, direct);
    } else if (rs == "a") {
        return viua::arch::Register_access::make_argument(index, direct);
    } else if (rs == "p") {
        return viua::arch::Register_access::make_parameter(index, direct);
    } else {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{ingredients.back(), Cause::Invalid_register_access}
            .add(ingredients.at(0))
            .add(ingredients.at(1))
            .add(ingredients.at(2))
            .aside("invalid register set specifier: "
                   + viua::support::string::quote_squares(rs.text))
            .note("valid register set specifiers are " + quote_fancy("l") + ", "
                  + quote_fancy("a") + ", and " + quote_fancy("p"));
    }
}
struct Instruction : Node {
    std::vector<Operand> operands;
};
struct Begin : Node {};
struct End : Node {};
}  // namespace ast
auto dump_nodes(std::vector<std::unique_ptr<ast::Node>> const& nodes) -> void
{
    if (nodes.empty()) {
        return;
    }

    auto const max_line = nodes.back()->leader.location.line;
    auto const pad_line = std::to_string(max_line).size();

    auto const max_char =
        std::max_element(nodes.begin(),
                         nodes.end(),
                         [](auto const& lhs, auto const& rhs) -> bool {
                             return lhs->leader.location.character
                                    < rhs->leader.location.character;
                         })
            ->get()
            ->leader.location.character;
    auto const max_size =
        std::max_element(nodes.begin(),
                         nodes.end(),
                         [](auto const& lhs, auto const& rhs) -> bool {
                             return lhs->leader.text.size()
                                    < rhs->leader.text.size();
                         })
            ->get()
            ->leader.text.size();
    auto const pad_location =
        1 + std::to_string(max_char).size() + std::to_string(max_size).size();

    auto const max_offset = nodes.back()->leader.location.offset;
    auto const pad_offset = std::to_string(max_offset).size();

    for (auto const& each : nodes) {
        auto const& leader = each->leader;
        auto token_kind    = viua::libs::lexer::to_string(leader.token);
        token_kind.resize(17, ' ');

        auto loc = std::to_string(leader.location.character) + '-'
                   + std::to_string(leader.location.character
                                    + leader.text.size() - 1);
        loc.resize(pad_location, ' ');

        auto off = "+" + std::to_string(leader.location.offset);
        off.resize(2 + pad_offset, ' ');

        std::cerr << "  " << token_kind;
        std::cerr << "  " << std::setw(pad_line) << leader.location.line << ':'
                  << loc;
        std::cerr << "  " << off;

        auto printable = std::string{};
        switch (leader.token) {
            using enum viua::libs::lexer::TOKEN;
        case SWITCH_TO_SECTION:
            printable = static_cast<ast::Section&>(*each).which();
            break;
        case DEFINE_LABEL:
            printable = static_cast<ast::Label&>(*each).name.text;
            break;
        case DECLARE_SYMBOL:
            printable = static_cast<ast::Symbol&>(*each).name.text;
            break;
        case ALLOCATE_OBJECT:
            printable = static_cast<ast::Object&>(*each).type.text;
            break;
        case OPCODE:
            printable = static_cast<ast::Instruction&>(*each).leader.text;
            break;
        default:
            break;
        }
        if (not printable.empty()) {
            std::cerr << "  " << printable;
        }

        std::cerr << "\n";
    }
}

auto did_you_mean(viua::libs::errors::compile_time::Error& e, std::string what)
    -> viua::libs::errors::compile_time::Error&
{
    return e.aside("did you mean " + quote_fancy(what) + "?");
}
auto did_you_mean(viua::libs::errors::compile_time::Error&& e, std::string what)
    -> viua::libs::errors::compile_time::Error
{
    did_you_mean(e, what);
    return e;
}

auto consume_token_of(
    viua::libs::lexer::TOKEN const tt,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (lexemes.front().token != tt) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        auto e = Error{lexemes.front(), Cause::Unexpected_token};
        {
            constexpr auto esc = viua::support::tty::send_escape_seq;
            constexpr auto q   = viua::support::string::quote_fancy;
            using viua::support::tty::ATTR_FONT_BOLD;
            using viua::support::tty::ATTR_FONT_NORMAL;

            auto const BOLD = std::string{esc(2, ATTR_FONT_BOLD)};
            auto const NORM = std::string{esc(2, ATTR_FONT_NORMAL)};

            e.aside("expected "
                    + q(BOLD + viua::libs::lexer::to_string(tt) + NORM));
        }
        throw e;
    }
    auto lx = lexemes.front();
    lexemes.remove_prefix(1);
    return lx;
}
auto consume_token_of(
    std::set<viua::libs::lexer::TOKEN> const ts,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (ts.count(lexemes.front().token) == 0) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        auto e = Error{lexemes.front(), Cause::Unexpected_token};
        {
            auto s = std::ostringstream{};
            s << "expected ";
            if (ts.size() > 1) {
                s << "one of: ";
            }

            constexpr auto esc = viua::support::tty::send_escape_seq;
            constexpr auto q   = viua::support::string::quote_fancy;
            using viua::support::tty::ATTR_FONT_BOLD;
            using viua::support::tty::ATTR_FONT_NORMAL;

            auto const BOLD = std::string{esc(2, ATTR_FONT_BOLD)};
            auto const NORM = std::string{esc(2, ATTR_FONT_NORMAL)};

            auto it = ts.begin();
            s << q(BOLD + viua::libs::lexer::to_string(*it) + NORM);
            while (++it != ts.end()) {
                s << ", ";
                s << q(BOLD + viua::libs::lexer::to_string(*it) + NORM);
            }

            e.aside(s.str());
        }
        throw e;
    }
    auto lx = lexemes.front();
    lexemes.remove_prefix(1);
    return lx;
}

auto look_ahead [[maybe_unused]] (
    viua::libs::lexer::TOKEN const tk,
    viua::support::vector_view<viua::libs::lexer::Lexeme> const& lexemes)
-> bool
{
    return (not lexemes.empty()) and (lexemes.front() == tk);
}
auto look_ahead [[maybe_unused]] (
    std::set<viua::libs::lexer::TOKEN> const ts,
    viua::support::vector_view<viua::libs::lexer::Lexeme> const& lexemes)
-> bool
{
    return (not lexemes.empty()) and (ts.count(lexemes.front().token) != 0);
}

auto consume_one_attr(viua::support::vector_view<Lexeme>& lexemes)
    -> ast::Node::attribute_type
{
    using viua::libs::lexer::TOKEN;
    auto key   = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
    auto value = key.make_synth();
    if (look_ahead(TOKEN::EQ, lexemes)) {
        consume_token_of(TOKEN::EQ, lexemes);
        value = consume_token_of({TOKEN::LITERAL_INTEGER,
                                  TOKEN::LITERAL_STRING,
                                  TOKEN::LITERAL_ATOM},
                                 lexemes);
    }
    return {std::move(key), std::move(value)};
}
auto consume_attrs(
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> std::vector<ast::Node::attribute_type>
{
    auto attrs = std::vector<ast::Node::attribute_type>{};

    /*
     * Empty attribute lists are not allowed.
     */
    using viua::libs::lexer::TOKEN;
    consume_token_of(TOKEN::ATTR_LIST_OPEN, lexemes);
    attrs.emplace_back(consume_one_attr(lexemes));
    while (not look_ahead(TOKEN::ATTR_LIST_CLOSE, lexemes)) {
        consume_token_of(TOKEN::COMMA, lexemes);
        attrs.emplace_back(consume_one_attr(lexemes));
    }
    consume_token_of(TOKEN::ATTR_LIST_CLOSE, lexemes);

    return attrs;
}

auto consume_switch_to_text(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Section>
{
    using viua::libs::lexer::TOKEN;
    auto ss         = std::make_unique<ast::Section>();
    auto const base = consume_token_of(TOKEN::SWITCH_TO_TEXT, lexemes);
    ss->leader      = base.make_synth(".section", TOKEN::SWITCH_TO_SECTION);
    ss->name        = base.make_synth("\".text\"", TOKEN::LITERAL_STRING);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_switch_to_rodata(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Section>
{
    using viua::libs::lexer::TOKEN;
    auto ss         = std::make_unique<ast::Section>();
    auto const base = consume_token_of(TOKEN::SWITCH_TO_RODATA, lexemes);
    ss->leader      = base.make_synth(".section", TOKEN::SWITCH_TO_SECTION);
    ss->name        = base.make_synth("\".rodata\"", TOKEN::LITERAL_STRING);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_switch_to_section(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Section>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Section>();
    ss->leader = consume_token_of(TOKEN::SWITCH_TO_SECTION, lexemes);
    ss->name   = consume_token_of(TOKEN::LITERAL_STRING, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_label(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Label>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Label>();
    ss->leader = consume_token_of(TOKEN::DEFINE_LABEL, lexemes);
    ss->name =
        consume_token_of({TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_object(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Object>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Object>();
    ss->leader = consume_token_of(TOKEN::ALLOCATE_OBJECT, lexemes);
    if (look_ahead(TOKEN::ATTR_LIST_OPEN, lexemes)) {
        ss->attributes = consume_attrs(lexemes);
    }
    ss->type = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
    while (not look_ahead(TOKEN::TERMINATOR, lexemes)) {
        ss->ctor.emplace_back(consume_token_of(
            {TOKEN::LITERAL_STRING, TOKEN::STAR, TOKEN::LITERAL_INTEGER},
            lexemes));
    }
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_symbol(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Symbol>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Symbol>();
    ss->leader = consume_token_of(TOKEN::DECLARE_SYMBOL, lexemes);
    if (look_ahead(TOKEN::ATTR_LIST_OPEN, lexemes)) {
        ss->attributes = consume_attrs(lexemes);
    }

    using viua::libs::errors::compile_time::Cause;
    using viua::libs::errors::compile_time::Error;
    try {
        ss->name = consume_token_of(
            {TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
    } catch (Error& e) {
        if (lexemes.size() < 3) {
            throw e;
        }
        if (lexemes.at(1) != TOKEN::DEFINE_LABEL) {
            throw e;
        }

        auto const& label = lexemes.at(2);
        if (label != TOKEN::LITERAL_ATOM and label != TOKEN::LITERAL_STRING) {
            throw e;
        }

        throw e.chain(Error{label, Cause::None}.note(
            ("did you mean " + quote_fancy(make_name_from_lexeme(label))
             + "?")));
    }
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_begin(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Begin>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::Begin>();
    ss->leader = consume_token_of(TOKEN::BEGIN, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_end(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::End>
{
    using viua::libs::lexer::TOKEN;
    auto ss    = std::make_unique<ast::End>();
    ss->leader = consume_token_of(TOKEN::END, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);
    return ss;
}
auto consume_instruction(viua::support::vector_view<Lexeme>& lexemes)
    -> std::unique_ptr<ast::Instruction>
{
    auto instruction = std::make_unique<ast::Instruction>();

    using viua::libs::lexer::TOKEN;
    try {
        instruction->leader = consume_token_of(TOKEN::OPCODE, lexemes);
    } catch (viua::libs::lexer::Lexeme const& e) {
        if (e.token != viua::libs::lexer::TOKEN::LITERAL_ATOM) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unknown_opcode, e.text};
        }

        using viua::libs::lexer::OPCODE_NAMES;
        using viua::support::string::levenshtein_filter;
        auto misspell_candidates = levenshtein_filter(e.text, OPCODE_NAMES);
        if (misspell_candidates.empty()) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unknown_opcode, e.text};
        }

        using viua::support::string::levenshtein_best;
        auto best_candidate =
            levenshtein_best(e.text, misspell_candidates, (e.text.size() / 2));
        if (best_candidate.second == e.text) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unknown_opcode, e.text};
        }

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw did_you_mean(Error{e, Cause::Unknown_opcode, e.text},
                           best_candidate.second);
    }

    /*
     * Special case for instructions with no operands. It is here to make
     * the loop that extracts the operands simpler.
     */
    if (lexemes.front() == TOKEN::TERMINATOR) {
        consume_token_of(TOKEN::TERMINATOR, lexemes);
        return instruction;
    }

    auto const is_fun_type_name = [](Lexeme const& f) -> bool {
        return (f == TOKEN::OPCODE) and FUNDAMENTAL_TYPE_NAMES.contains(f.text);
    };

    while ((not lexemes.empty()) and lexemes.front() != TOKEN::END) {
        if (lexemes.front().token == TOKEN::END) {
            break;
        }

        auto operand = ast::Operand{};

        /*
         * Attributes come before the element they describe, so let's try to
         * parse them before an operand.
         */
        if (lexemes.front() == TOKEN::ATTR_LIST_OPEN) {
            operand.attributes = consume_attrs(lexemes);
        }

        /*
         * Consume the operand: void, register access, a literal value. This
         * will supply some value for the instruction to work on. This chain
         * of if-else should handle valid operands - and ONLY operands, not
         * their separators.
         */
        if (lexemes.front() == TOKEN::VOID) {
            operand.ingredients.push_back(
                consume_token_of(TOKEN::VOID, lexemes));
        } else if (look_ahead(TOKEN::DOLLAR, lexemes)) {
            auto const leader = consume_token_of(TOKEN::DOLLAR, lexemes);
            auto index        = Lexeme{};
            try {
                index = consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
            } catch (viua::libs::lexer::Lexeme const& e) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{e, Cause::Invalid_register_access}
                    .add(leader)
                    .aside("register index must be an integer");
            }
            try {
                auto const n = std::stoul(index.text);
                if (n > viua::arch::MAX_REGISTER_INDEX) {
                    throw std::out_of_range{""};
                }
            } catch (std::out_of_range const&) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{index, Cause::Invalid_register_access}.aside(
                    "register index range is 0-"
                    + std::to_string(static_cast<uintmax_t>(
                        viua::arch::MAX_REGISTER_INDEX)));
            }

            operand.ingredients.push_back(leader);
            operand.ingredients.push_back(index);

            if (look_ahead(TOKEN::DOT, lexemes)) {
                operand.ingredients.push_back(
                    consume_token_of(TOKEN::DOT, lexemes));
                operand.ingredients.push_back(
                    consume_token_of(TOKEN::LITERAL_ATOM, lexemes));
            } else {
                /*
                 * We did not encounter a dot and the explicit specifier of
                 * register set, so let's consider the only other valid
                 * following tokens.
                 */
                if (not look_ahead({TOKEN::COMMA, TOKEN::TERMINATOR},
                                   lexemes)) {
                    /*
                     * Yeah, the token stream did not match expectations. Abort.
                     */
                    consume_token_of({TOKEN::COMMA, TOKEN::DOT}, lexemes);
                }
            }
        } else if (look_ahead(TOKEN::AT, lexemes)) {
            auto const access = consume_token_of(TOKEN::AT, lexemes);
            auto label        = viua::libs::lexer::Lexeme{};
            try {
                label = consume_token_of(
                    {TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
            } catch (viua::libs::errors::compile_time::Error& e) {
                throw e.add(access).aside("label must be an atom or a string");
            }
            operand.ingredients.push_back(access);
            operand.ingredients.push_back(label);
        } else if (lexemes.front() == TOKEN::LITERAL_INTEGER) {
            auto const value =
                consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_FLOAT) {
            auto const value = consume_token_of(TOKEN::LITERAL_FLOAT, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_STRING) {
            auto const value = consume_token_of(TOKEN::LITERAL_STRING, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_ATOM) {
            auto const value = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
            operand.ingredients.push_back(value);
        } else if (lexemes.front() == TOKEN::LITERAL_ATOM) {
            auto const value = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
            operand.ingredients.push_back(value);
        } else if (is_fun_type_name(lexemes.front())) {
            /*
             * Ensure that fundamental type names are always represented using
             * "atom" tokens. This is necessary because some of them are
             * duplicating opcode names (eg, "double").
             */
            auto value  = consume_token_of(TOKEN::OPCODE, lexemes);
            value.token = TOKEN::LITERAL_ATOM;
            operand.ingredients.push_back(value);
        } else {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{
                lexemes.front(), Cause::Unexpected_token, "cannot consume"};
        }

        instruction->operands.push_back(std::move(operand));

        /*
         * Consume either a comma (meaning that there will be some more
         * operands), or a terminator (meaning that there will be no more
         * operands).
         */
        if (lexemes.front() == TOKEN::COMMA) {
            auto const comma = consume_token_of(TOKEN::COMMA, lexemes);

            if (look_ahead(TOKEN::TERMINATOR, lexemes)) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{comma,
                            Cause::None,
                            "expected an operand to follow a comma"};
            }

            continue;
        }
        if (lexemes.front() == TOKEN::TERMINATOR) {
            consume_token_of(TOKEN::TERMINATOR, lexemes);
            break;
        }

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{lexemes.front(), Cause::Unexpected_token}.note(
            "expected a comma, or a newline");
    }

    return instruction;
}
auto parse(viua::support::vector_view<Lexeme> lexemes)
    -> std::vector<std::unique_ptr<ast::Node>>
{
    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};

    auto attributes = std::vector<ast::Node::attribute_type>{};

    while (not lexemes.empty()) {
        auto const& each = lexemes.front();

        using viua::libs::lexer::TOKEN;
        switch (each.token) {
        case TOKEN::SWITCH_TO_TEXT:
            nodes.emplace_back(consume_switch_to_text(lexemes));
            break;
        case TOKEN::SWITCH_TO_RODATA:
            nodes.emplace_back(consume_switch_to_rodata(lexemes));
            break;
        case TOKEN::SWITCH_TO_SECTION:
            nodes.emplace_back(consume_switch_to_section(lexemes));
            break;
        case TOKEN::DEFINE_LABEL:
            nodes.emplace_back(consume_label(lexemes));
            break;
        case TOKEN::ALLOCATE_OBJECT:
            nodes.emplace_back(consume_object(lexemes));
            break;
        case TOKEN::DECLARE_SYMBOL:
            nodes.emplace_back(consume_symbol(lexemes));
            break;
        case TOKEN::BEGIN:
            nodes.emplace_back(consume_begin(lexemes));
            break;
        case TOKEN::END:
            nodes.emplace_back(consume_end(lexemes));
            break;
        case TOKEN::ATTR_LIST_OPEN:
            attributes = consume_attrs(lexemes);
            [[fallthrough]];
        case TOKEN::OPCODE:
            nodes.emplace_back(consume_instruction(lexemes));
            nodes.back()->attributes = std::move(attributes);
            break;
        default:
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            constexpr auto esc = viua::support::tty::send_escape_seq;
            constexpr auto q   = viua::support::string::quote_fancy;
            using viua::support::tty::ATTR_FONT_BOLD;
            using viua::support::tty::ATTR_FONT_NORMAL;

            auto const BOLD = std::string{esc(2, ATTR_FONT_BOLD)};
            auto const NORM = std::string{esc(2, ATTR_FONT_NORMAL)};

            auto m = q(BOLD + viua::libs::lexer::to_string(each.token) + NORM)
                     + " cannot appear at top level";
            throw Error{each, Cause::Unexpected_token, std::move(m)}.note(
                "refer to viua-asm-lang(1) for more information");
        }
    }

    return nodes;
}

auto make_symbol(std::string_view const name,
                 std::vector<uint8_t>& string_table) -> Elf64_Sym
{
    auto const name_off =
        name.empty() ? 0 : save_string_to_strtab(string_table, name);

    auto sym    = Elf64_Sym{};
    sym.st_name = name_off;
    return sym;
}
auto make_symbol(viua::libs::lexer::Lexeme const& name,
                 std::vector<uint8_t>& string_table) -> Elf64_Sym
{
    auto const& unquoted_name = make_name_from_lexeme(name);
    auto const name_off =
        unquoted_name.empty()
            ? 0
            : save_string_to_strtab(string_table, unquoted_name);

    auto sym    = Elf64_Sym{};
    sym.st_name = name_off;
    return sym;
}

using Decl_map =
    std::vector<std::pair<ast::Symbol const&, ast::Section const&>>;

auto save_declared_symbols(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                           std::vector<uint8_t>& string_table,
                           std::vector<Elf64_Sym>& symbol_table,
                           std::map<std::string, size_t>& symbol_map,
                           Decl_map& declarations) -> void
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};
    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        if (each->leader.token != TOKEN::DECLARE_SYMBOL) {
            continue;
        }
        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }

        auto const& sym = static_cast<ast::Symbol const&>(*each);

        auto type = (active_section == SECTION::TEXT) ? STT_FUNC : STT_OBJECT;
        auto binding    = (active_section == SECTION::TEXT) ? STB_GLOBAL
                                                            : STB_LOCAL;
        auto visibility = STV_DEFAULT;

        if (sym.has_attr("local") and sym.has_attr("global")) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto const glo = sym.has_attr("global").value();
            auto const loc = sym.has_attr("local").value();
            auto const [earlier, later] =
                (glo.first.location.offset < loc.first.location.offset)
                    ? std::pair{glo, loc}
                    : std::pair{loc, glo};

            auto e = Error{later.first,
                           Cause::None,
                           "symbols cannot be explicitly local and global"};
            e.add(earlier.first);
            e.aside("remove one of the tags");
            throw e;
        } else if (sym.has_attr("local")) {
            binding = STB_LOCAL;
        } else if (sym.has_attr("global")) {
            binding = STB_GLOBAL;
        }

        if (sym.has_attr("hidden")) {
            visibility = STV_HIDDEN;
        }

        if (binding == STB_GLOBAL and type == STT_OBJECT
            and visibility != STV_HIDDEN) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            auto e = Error{sym.has_attr("global")
                               .value_or(std::pair{each->leader, std::nullopt})
                               .first,
                           Cause::None,
                           "object symbols cannot be globally visible"};
            e.chain(Error{activator->leader, Cause::None}.note(
                "section activated here"));

            if (auto const g = sym.has_attr("global"); g) {
                e.add(each->leader);
                e.aside("remove tag or change it to explicit "
                        + quote_fancy("local"));
            }

            throw e;
        }

        auto symbol     = make_symbol(sym.name, string_table);
        symbol.st_info  = static_cast<uint8_t>(ELF64_ST_INFO(binding, type));
        symbol.st_other = static_cast<uint8_t>(visibility);

        /*
         * Leave size and offset of the symbol empty since we do not have this
         * information yet. It will only be available after the whole ELF is
         * emitted.
         *
         * For symbols marked as [[extern]] st_value will be LEFT EMPTY after
         * the assembler exits, as a signal to the linker that this symbol was
         * defined in a different module and needs to be resolved.
         */
        symbol.st_size  = 0;
        symbol.st_value = 0;

        /*
         * Section header table index (see elf(5) for st_shndx) is filled
         * out later, since at this point we do not have this information.
         *
         * For variables it will be the index of .rodata.
         * For functions it will be the index of .text.
         */
        symbol.st_shndx = 0;

        viua::libs::stage::record_symbol(
            make_name_from_lexeme(sym.name), symbol, symbol_table, symbol_map);
        declarations.emplace_back(sym, *activator);
    }
}

auto save_objects(std::vector<std::unique_ptr<ast::Node>>& nodes,
                  std::vector<uint8_t>& rodata_buf,
                  std::vector<uint8_t>& string_table,
                  std::vector<Elf64_Sym>& symbol_table,
                  std::map<std::string, size_t>& symbol_map,
                  Decl_map const& declared_symbols) -> void
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};

    auto active_label = std::string{};
    auto labeller     = std::experimental::observer_ptr<ast::Label const>{};

    auto active_symbol = std::experimental::observer_ptr<Elf64_Sym>{};

    for (auto& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        auto const is_label = (each->leader.token == TOKEN::DEFINE_LABEL);
        auto const is_alloc = (each->leader.token == TOKEN::ALLOCATE_OBJECT);
        auto const is_instr = (each->leader.token == TOKEN::OPCODE);
        if (auto const is_interesting = is_label or is_alloc or is_instr;
            not is_interesting) {
            continue;
        }

        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }

        if (is_label and (active_section == SECTION::RODATA)) {
            auto const& lab = static_cast<ast::Label&>(*each);
            labeller.reset(&lab);

            // FIXME add proper unquote
            active_label =
                (lab.name.token == viua::libs::lexer::TOKEN::LITERAL_STRING)
                    ? lab.name.text.substr(1, lab.name.text.size() - 2)
                    : lab.name.text;

            if (not symbol_map.contains(active_label)) {
                auto local_sym     = make_symbol(active_label, string_table);
                local_sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
                local_sym.st_other = STV_DEFAULT;
                auto sym_ndx       = viua::libs::stage::record_symbol(
                    active_label, local_sym, symbol_table, symbol_map);
                active_symbol.reset(&symbol_table.at(sym_ndx));
            } else {
                auto& sym = symbol_table.at(symbol_map.at(active_label));
                if (ELF64_ST_TYPE(sym.st_info) != STT_OBJECT) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;

                    auto const& sym_name = labeller->name;

                    auto e = Error{each->leader,
                                   Cause::None,
                                   "invalid definition of symbol "
                                       + quote_fancy(sym_name.text) + "..."};
                    e.add(sym_name, true);
                    e.chain(Error{activator->leader,
                                  Cause::None,
                                  ("...in section " + activator->name.text)});

                    auto const& [decl_sym, decl_sec] = *std::ranges::find_if(
                        declared_symbols,
                        [&name = labeller->name.text](auto const& dl) -> bool {
                            return (dl.first.name.text == name);
                        });
                    e.chain(Error{decl_sym.leader,
                                  Cause::None,
                                  "previously declared here..."}
                                .add(decl_sym.name, true));
                    e.chain(Error{decl_sec.leader,
                                  Cause::None,
                                  ("...in section " + decl_sec.name.text)});

                    throw e;
                }

                active_symbol.reset(&sym);
            }

            std::cerr << "recording object label " << active_label << " at "
                      << "[.rodata+0x" << std::hex << std::setfill('0')
                      << std::setw(16) << rodata_buf.size() << std::dec
                      << std::setfill(' ') << "]\n";

            active_symbol->st_value = rodata_buf.size();
            continue;
        }

        if (is_alloc and (active_section != SECTION::RODATA)) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            throw Error{each->leader, Cause::None, "FUCKUP"};
        } else if (is_alloc) {
            auto const& alc = static_cast<ast::Object&>(*each);
            std::cerr << "allocating " << alc.type.text << " under label "
                      << active_label << "\n";

            if (alc.type.text == "string") {
                auto s = std::string{};
                for (auto i = size_t{0}; i < alc.ctor.size(); ++i) {
                    auto& each = alc.ctor.at(i);

                    using enum viua::libs::lexer::TOKEN;
                    if (each.token == LITERAL_STRING) {
                        auto tmp = each.text;
                        tmp      = tmp.substr(1, tmp.size() - 2);
                        tmp      = viua::support::string::unescape(tmp);
                        s += tmp;
                    } else if (each.token == LITERAL_INTEGER) {
                        auto& star = alc.ctor.at(i + 1);
                        if (star.token != STAR) {
                            using viua::libs::errors::compile_time::Cause;
                            using viua::libs::errors::compile_time::Error;

                            throw Error{each,
                                        Cause::Invalid_operand,
                                        "an integer must be followed by a star "
                                        "in string concatenation"}
                                .add(star)
                                .add(alc.type);
                        }
                        continue;
                    } else if (each.token == STAR) {
                        if (i == 0) {
                            using viua::libs::errors::compile_time::Cause;
                            using viua::libs::errors::compile_time::Error;

                            throw Error{each,
                                        Cause::Invalid_operand,
                                        "in string concatenation"}
                                .aside("left-hand side must be an "
                                       "positive integer, but star was the "
                                       "first token");
                        }

                        auto const& n = alc.ctor.at(i - 1);
                        if (n.token != LITERAL_INTEGER) {
                            using viua::libs::errors::compile_time::Cause;
                            using viua::libs::errors::compile_time::Error;

                            throw Error{n,
                                        Cause::Invalid_operand,
                                        "in string concatenation"}
                                .add(each)
                                .aside("left-hand side must be an "
                                       "positive integer");
                        }

                        auto const& part = alc.ctor.at(++i);
                        if (part.token != LITERAL_STRING) {
                            using viua::libs::errors::compile_time::Cause;
                            using viua::libs::errors::compile_time::Error;

                            throw Error{part,
                                        Cause::Invalid_operand,
                                        "in string concatenation"}
                                .add(each)
                                .aside(
                                    "right-hand side must be a string literal");
                        }

                        auto tmp = part.text;
                        tmp      = tmp.substr(1, tmp.size() - 2);
                        tmp      = viua::support::string::unescape(tmp);

                        using viua::support::ston;
                        auto const r = std::ranges::iota_view<size_t>(0ul)
                                       | std::views::take(ston<size_t>(n.text));
                        std::ranges::for_each(r,
                                              [&s, &tmp](auto) { s += tmp; });
                    } else {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        throw Error{each,
                                    Cause::Invalid_operand,
                                    "expected an integer or a string"};
                    }
                }

                auto const value_off = save_buffer_to_rodata(rodata_buf, s);
                if (active_symbol) {
                    active_symbol->st_value = value_off;
                    active_symbol->st_size =
                        (rodata_buf.size() - active_symbol->st_value);
                }

                std::cerr << "allocated " << active_symbol->st_size
                          << " byte(s) of " << alc.type.text << " under label "
                          << active_label << " at "
                          << "[.rodata+0x" << std::hex << std::setfill('0')
                          << std::setw(16) << active_symbol->st_value
                          << std::dec << std::setfill(' ') << "]\n";
                active_symbol.reset();
            }
        }

        if (is_instr) {
            /*
             * Some instructions may carry "long immediates" ie, values which do
             * not fit into the instruction itself and must be recovered from
             * .rodata at runtime.
             *
             * These long immediates must be put into .rodata and converted into
             * loads.
             */

            auto& instr   = static_cast<ast::Instruction&>(*each);
            auto saved_at = size_t{0};

            if (instr.leader == "atom" or instr.leader == "g.atom") {
                auto const lx = instr.operands.back().ingredients.front();
                using enum viua::libs::lexer::TOKEN;
                if (lx.token == LITERAL_STRING or lx.token == LITERAL_ATOM) {
                    auto s = lx.text;
                    if (lx.token == LITERAL_STRING) {
                        s = s.substr(1, s.size() - 2);
                        s = viua::support::string::unescape(s);
                    }

                    auto const value_off = save_buffer_to_rodata(rodata_buf, s);

                    auto symbol     = Elf64_Sym{};
                    symbol.st_name  = 0; /* anonymous symbol */
                    symbol.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
                    symbol.st_other = STV_DEFAULT;

                    symbol.st_value = value_off;
                    symbol.st_size  = s.size();

                    /*
                     * Section header table index (see elf(5) for st_shndx) is
                     * filled out later, since at this point we do not have this
                     * information.
                     *
                     * For variables it will be the index of .rodata.
                     * For functions it will be the index of .text.
                     */
                    symbol.st_shndx = 0;

                    std::cerr << "allocated " << symbol.st_size
                              << " byte(s) of string anonymous at "
                              << "[.rodata+0x" << std::hex << std::setfill('0')
                              << std::setw(16) << symbol.st_value << std::dec
                              << std::setfill(' ') << "]\n";

                    saved_at = viua::libs::stage::record_symbol(
                        "", symbol, symbol_table, symbol_map);
                } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
                    auto const label = instr.operands.back().ingredients.back();
                    try {
                        saved_at = symbol_map.at(label.text);
                    } catch (std::out_of_range const&) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto e = Error{label,
                                       Cause::Unknown_label,
                                       quote_fancy(label.text)};
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
                                did_you_mean(e, best_candidate.second);
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
                        did_you_mean(e, '@' + lx.text);
                    }

                    throw e;
                }
            } else if (instr.leader == "arodp" or instr.leader == "g.arodp") {
                auto const lx = instr.operands.back().ingredients.front();
                using enum viua::libs::lexer::TOKEN;
                if (lx.token == viua::libs::lexer::TOKEN::AT) {
                    auto const label = instr.operands.back().ingredients.back();
                    try {
                        saved_at = symbol_map.at(label.text);
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
                                did_you_mean(e, best_candidate.second);
                            }
                        }

                        throw e;
                    }
                } else {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;

                    auto e = Error{lx,
                                   Cause::Invalid_operand,
                                   "expected a label reference"};
                    if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                        did_you_mean(e, '@' + lx.text);
                    }
                    throw e;
                }
            } else if (instr.leader == "double" or instr.leader == "g.double") {
                auto const lx = instr.operands.back().ingredients.front();
                using enum viua::libs::lexer::TOKEN;
                if (lx.token == LITERAL_FLOAT) {
                    constexpr auto SIZE_OF_DOUBLE_PRECISION_FLOAT = size_t{8};
                    auto const f                                  = std::stod(
                        instr.operands.back().ingredients.front().text);
                    auto s = std::string(SIZE_OF_DOUBLE_PRECISION_FLOAT, '\0');
                    memcpy(s.data(), &f, SIZE_OF_DOUBLE_PRECISION_FLOAT);
                    auto const value_off = save_buffer_to_rodata(rodata_buf, s);

                    auto symbol     = Elf64_Sym{};
                    symbol.st_name  = 0; /* anonymous symbol */
                    symbol.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
                    symbol.st_other = STV_DEFAULT;

                    symbol.st_value = value_off;
                    symbol.st_size  = s.size();

                    /*
                     * Section header table index (see elf(5) for st_shndx) is
                     * filled out later, since at this point we do not have this
                     * information.
                     *
                     * For variables it will be the index of .rodata.
                     * For functions it will be the index of .text.
                     */
                    symbol.st_shndx = 0;

                    std::cerr << "allocated " << symbol.st_size
                              << " byte(s) of double anonymous at "
                              << "[.rodata+0x" << std::hex << std::setfill('0')
                              << std::setw(16) << symbol.st_value << std::dec
                              << std::setfill(' ') << "]\n";

                    saved_at = viua::libs::stage::record_symbol(
                        "", symbol, symbol_table, symbol_map);
                } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
                    auto const label = instr.operands.back().ingredients.back();
                    try {
                        saved_at = symbol_map.at(label.text);
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
                                did_you_mean(e, best_candidate.second);
                            }
                        }

                        throw e;
                    }
                } else {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;

                    auto e = Error{lx, Cause::Invalid_operand}.aside(
                        "expected double literal, or a label reference");

                    if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                        did_you_mean(e, '@' + lx.text);
                    }

                    throw e;
                }
            }

            /*
             * Only rewrite the instruction if needed. We can safely use the
             * object offset for this since no value is ever saved at address
             * zero in the .rodata section.
             */
            if (saved_at) {
                auto synth_op  = instr.operands.back().ingredients.front();
                synth_op.token = TOKEN::LITERAL_INTEGER;
                synth_op.text  = std::to_string(saved_at) + 'u';

                instr.operands.back().ingredients.clear();
                instr.operands.back().ingredients.push_back(synth_op);
            }
        }
    }
}

auto cache_function_labels(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                           std::vector<uint8_t>& string_table,
                           std::vector<Elf64_Sym>& symbol_table,
                           std::map<std::string, size_t>& symbol_map) -> void
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};

    auto active_label = std::string{};
    auto labeller     = std::experimental::observer_ptr<ast::Label const>{};

    auto active_symbol = std::experimental::observer_ptr<Elf64_Sym>{};

    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        auto const is_label = (each->leader.token == TOKEN::DEFINE_LABEL);
        if (auto const is_interesting = is_label; not is_interesting) {
            continue;
        }

        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }
        if (active_section == SECTION::RODATA) {
            continue;
        }

        if (is_label) {
            auto const& lab = static_cast<ast::Label&>(*each);
            labeller.reset(&lab);
            active_label = make_name_from_lexeme(lab.name);

            if (not symbol_map.contains(active_label)) {
                auto local_sym     = make_symbol(active_label, string_table);
                local_sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_FUNC);
                local_sym.st_other = STV_HIDDEN;
                auto sym_ndx       = viua::libs::stage::record_symbol(
                    active_label, local_sym, symbol_table, symbol_map);
                active_symbol.reset(&symbol_table.at(sym_ndx));
            } else {
                active_symbol.reset(
                    &symbol_table.at(symbol_map.at(active_label)));
            }

            std::cerr << "caching "
                      << ((active_symbol->st_other == STV_DEFAULT) ? "function"
                                                                   : "jump")
                      << " label " << active_label << "\n";
            continue;
        }
    }
}

auto operand_or_throw(ast::Instruction const& insn, size_t const index)
    -> ast::Operand const&
{
    try {
        return insn.operands.at(index);
    } catch (std::out_of_range const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{insn.leader,
                    Cause::Too_few_operands,
                    ("operand " + std::to_string(index) + " not found")};
    }
}
auto emit_instruction(ast::Instruction const insn)
    -> viua::arch::instruction_type
{
    using viua::arch::opcode_type;
    using viua::arch::ops::FORMAT;
    using viua::arch::ops::FORMAT_MASK;
    using viua::arch::ops::OPCODE;

    auto opcode = opcode_type{};
    try {
        opcode = viua::arch::ops::parse_opcode(insn.leader.text);
    } catch (std::invalid_argument const&) {
        auto const e = insn.leader;

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
        auto const imm = insn.operands.at(2).ingredients.front();
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
            auto val = uint32_t{};
            if (is_unsigned) {
                val = viua::support::ston<uint32_t>(imm.text);
            } else {
                auto const tmp = viua::support::ston<int32_t>(imm.text);
                memcpy(&val, &tmp, sizeof(tmp));
            }

            auto const fits = is_unsigned ? fits_in_unsigned_r_immediate(val)
                                          : fits_in_signed_r_immediate(val);
            if (not fits) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{imm,
                            Cause::Value_out_of_range,
                            "value does not fit into 24-bit wide immediate"};
            }

            return viua::arch::ops::R{opcode,
                                      insn.operands.at(0).make_access(),
                                      insn.operands.at(1).make_access(),
                                      val}
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
            insn.leader, Cause::Unknown_opcode, "cannot emit instruction"};
    }
}

using Text = std::vector<viua::arch::instruction_type>;
auto expand_li(ast::Instruction const& raw, bool const force_full = false)
    -> Text
{
    auto const& raw_value  = raw.operands.at(1).ingredients.front();
    auto const is_unsigned = (raw_value.text.back() == 'u');
    if (is_unsigned and raw_value.text.starts_with('-')
        and raw_value.text != "-1u") {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{raw_value, Cause::Value_out_of_range, "negative unsigned"}
            .note("the only signed value allowed in this context "
                  "is -1, and\n"
                  "it is used a symbol for maximum unsigned "
                  "value");
    }

    auto value = uint64_t{};
    try {
        if (is_unsigned) {
            value = viua::support::ston<uint64_t>(raw_value.text);
        } else {
            auto const tmp = viua::support::ston<int64_t>(raw_value.text);
            memcpy(&value, &tmp, sizeof(tmp));
        }
    } catch (std::out_of_range const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{raw_value, Cause::Invalid_operand, "value out of range"}
            .add(raw.leader);
    } catch (std::invalid_argument const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{raw_value, Cause::Invalid_operand, "expected integer"}.add(
            raw.leader);
    }

    using viua::libs::assembler::to_loading_parts_unsigned;
    auto const [hi, lo]  = to_loading_parts_unsigned(value);
    auto const is_greedy = (raw.leader.text.find("g.") == 0);
    auto const full_form = raw.has_attr("full") or force_full;

    std::cerr << "EXPANDING" << (is_unsigned ? " UNSIGNED" : "") << " LI OF "
              << raw_value.text << " => " << std::hex << std::setfill('0')
              << std::setw(16) << value << std::dec << std::setfill(' ')
              << " (";
    if (is_unsigned) {
        std::cerr << value;
    } else {
        std::cerr << static_cast<int64_t>(value);
    }
    std::cerr << ")"
              << "\n";
    std::cerr << "  HI " << std::hex << std::setfill('0') << std::setw(8) << hi
              << "\n";
    std::cerr << "  LO " << std::setw(8) << lo << "\n";
    std::cerr << std::setfill(' ') << std::dec;

    auto const important_hi = is_unsigned ? hi
                                          : (hi != static_cast<uint32_t>(-1));

    auto const fits_in_addi = is_unsigned ? fits_in_unsigned_r_immediate(lo)
                                          : fits_in_signed_r_immediate(lo);

    auto const needs_leader = full_form or important_hi or (not fits_in_addi);

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
    auto cooked = Text{};

    if (needs_leader) {
        using namespace std::string_literals;
        auto synth        = raw;
        synth.leader.text = ((lo or is_greedy) ? "g.lui" : "lui");
        if (is_unsigned) {
            synth.leader.text += 'u';
        }

        auto hi_literal = std::array<char, 2 + 8 + 1>{"0x"};
        std::to_chars(hi_literal.begin() + 2, hi_literal.end(), hi, 16);

        synth.operands.at(1).ingredients.front().text =
            std::string{hi_literal.data()};
        cooked.push_back(emit_instruction(synth));
    }

    /*
     * In the second step we use LLI to load the lower word if we are dealing
     * with a long immediate; or ADDI if we have a short immediate to deal with.
     */
    if (needs_leader) {
        auto synth        = raw;
        synth.leader.text = (is_greedy ? "g.lli" : "lli");

        auto const& lx = raw.operands.front().ingredients.at(1);

        using viua::libs::lexer::TOKEN;
        {
            auto dst = ast::Operand{};
            dst.ingredients.push_back(lx.make_synth("$", TOKEN::DOLLAR));
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

        cooked.push_back(emit_instruction(synth));
    } else {
        using namespace std::string_literals;
        auto synth        = raw;
        synth.leader.text = (is_greedy ? "g." : "") + "addi"s;
        if (is_unsigned) {
            synth.leader.text += 'u';
        }

        /*
         * Copy the last element (ie, the immediate operand) to easily reuse it.
         */
        synth.operands.push_back(synth.operands.back());

        /*
         * If the first part of the load (the high 36 bits) was zero then it
         * means we don't have anything to add to so the source (left-hand side
         * operand) should be void ie, the default value.
         */
        synth.operands.at(1).ingredients.front().text = "void";
        synth.operands.at(1).ingredients.front().token =
            viua::libs::lexer::TOKEN::VOID;

        cooked.push_back(emit_instruction(synth));
    }

    std::cerr << "        cooked into " << cooked.size() << " op(s)\n";

    return cooked;
}
auto expand_delete(ast::Instruction const& raw) -> Text
{
    using namespace std::string_literals;
    auto synth        = raw;
    synth.leader      = raw.leader;
    synth.leader.text = (raw.leader.text.find("g.") == 0 ? "g." : "") + "move"s;

    // FIXME comment this to get a good example of borked-because-synth error
    // report
    synth.operands.push_back(synth.operands.front());

    synth.operands.front().ingredients.front().text = "void";
    synth.operands.front().ingredients.resize(1);

    return {emit_instruction(synth)};
}
auto expand_flow_control(ast::Instruction const& raw,
                         std::vector<Elf64_Sym>& symbol_table,
                         std::map<std::string, size_t> const& symbol_map,
                         Decl_map const& decl_map) -> Text
{
    auto cooked = Text{};

    auto const target = raw.operands.back();
    auto jmp_offset   = target;
    {
        jmp_offset = ast::Operand{};

        using viua::libs::lexer::TOKEN;
        auto const& lx = target.ingredients.front();
        jmp_offset.ingredients.push_back(lx.make_synth("$", TOKEN::DOLLAR));
        jmp_offset.ingredients.push_back(
            lx.make_synth("253", TOKEN::LITERAL_INTEGER));
        jmp_offset.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
        jmp_offset.ingredients.push_back(
            lx.make_synth("l", TOKEN::LITERAL_ATOM));
    }

    auto li = ast::Instruction{};
    {
        li.leader      = raw.leader;
        li.leader.text = "g.li";

        li.operands.push_back(jmp_offset);
        li.operands.push_back(raw.operands.back());

        using viua::libs::lexer::TOKEN;

        auto const sym_id   = raw.operands.back().ingredients.front();
        auto const sym_name = make_name_from_lexeme(sym_id);
        if (symbol_map.count(sym_name) == 0) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto const cause = (raw.leader == "if")
                                   ? Cause::Jump_to_undefined_label
                                   : Cause::Call_to_undefined_function;

            throw Error{sym_id, cause, quote_fancy(sym_name)}.add(raw.leader);
        }

        auto const sym_off = symbol_map.at(sym_name);
        auto const sym     = symbol_table.at(sym_off);
        if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            throw Error{sym_id, Cause::Invalid_reference, quote_fancy(sym_name)}
                .aside("label " + quote_fancy(sym_name)
                       + " does not point into .text section")
                .add(raw.leader);
        }

        auto const is_jump_label = (ELF64_ST_BIND(sym.st_info) == STB_LOCAL)
                                   and (sym.st_other == STV_HIDDEN);
        if (not is_jump_label) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto const& [decl_sym, decl_sec] = *std::ranges::find_if(
                decl_map, [&name = sym_name](auto const& dl) -> bool {
                    return (dl.first.name.text == name);
                });

            throw Error{
                sym_id,
                Cause::Invalid_reference,
                quote_fancy(sym_name) + " cannot be used as a jump target"}
                .aside("label " + quote_fancy(sym_name)
                       + " defined as callable")
                .add(raw.leader)
                .chain(Error{decl_sym.name, Cause::None}
                           .note("declared here")
                           .add(decl_sym.leader));
        }

        li.operands.back().ingredients.front().text =
            (std::to_string(sym_off) + 'u');
        li.operands.back().ingredients.front().token = TOKEN::LITERAL_INTEGER;
    }
    std::ranges::copy(expand_li(li, true), std::back_inserter(cooked));

    auto jmp = ast::Instruction{};
    {
        jmp.leader = raw.leader;
        jmp.operands.push_back(raw.operands.front());
        jmp.operands.push_back(jmp_offset);
    }
    cooked.push_back(emit_instruction(jmp));

    std::cerr << "        cooked into " << cooked.size() << " ops\n";

    return cooked;
}
auto expand_call(ast::Instruction const& raw,
                 std::vector<Elf64_Sym>& symbol_table,
                 std::map<std::string, size_t> const& symbol_map) -> Text
{
    auto cooked = Text{};

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
     *      li $_, fn_tbl_offset(foo)
     *      call $1, $_
     */
    auto const ret = raw.operands.front();
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
        fn_offset.ingredients.push_back(lx.make_synth("$", TOKEN::DOLLAR));
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
        li.leader      = raw.leader;
        li.leader.text = "g.li";

        li.operands.push_back(fn_offset);
        li.operands.push_back(fn_offset);

        using viua::libs::lexer::TOKEN;

        auto const sym_id   = raw.operands.back().ingredients.front();
        auto const sym_name = make_name_from_lexeme(sym_id);
        if (symbol_map.count(sym_name) == 0) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto const cause = (raw.leader == "if")
                                   ? Cause::Jump_to_undefined_label
                                   : Cause::Call_to_undefined_function;

            throw Error{sym_id, cause, quote_fancy(sym_name)}.add(raw.leader);
        }

        auto const sym_off = symbol_map.at(sym_name);
        auto const sym     = symbol_table.at(sym_off);
        if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            throw Error{sym_id, Cause::Invalid_reference, quote_fancy(sym_name)}
                .aside("label " + quote_fancy(sym_name)
                       + " does not point into .text section")
                .add(raw.leader);
        }

        auto const is_jump_label = (ELF64_ST_BIND(sym.st_info) == STB_LOCAL)
                                   and (sym.st_other == STV_HIDDEN);
        if (is_jump_label) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            throw Error{sym_id,
                        Cause::Invalid_reference,
                        quote_fancy(sym_name) + " cannot be used as a callable"}
                .aside("label " + quote_fancy(sym_name)
                       + " defined as a jump target")
                .add(raw.leader);
        }

        li.operands.back().ingredients.front().text =
            (std::to_string(sym_off) + 'u');
        li.operands.back().ingredients.front().token = TOKEN::LITERAL_INTEGER;
    }
    std::ranges::copy(expand_li(li, true), std::back_inserter(cooked));

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
        call.leader = raw.leader;
        call.operands.push_back(ret);
        call.operands.push_back(fn_offset);
    }
    cooked.push_back(emit_instruction(call));

    std::cerr << "        cooked into " << cooked.size() << " ops\n";

    return cooked;
}
auto expand_atom(ast::Instruction const& raw) -> Text
{
    auto cooked = Text{};

    /*
     * Synthesize loading the atom offset first. It will emit a
     * sequence of instructions that will load the offset of the
     * data, which will then be used by the ATOM instruction to create the
     * object.
     */
    auto li = raw;
    {
        li.leader      = raw.leader;
        li.leader.text = "g.li";
    }
    std::ranges::copy(expand_li(li, true), std::back_inserter(cooked));

    /*
     * Then, synthesize the actual ATOM instruction. This means
     * simply replacing the literal atom (or a reference to one) with the
     * register containing its loaded offset ie, changing this code:
     *
     *      atom $42, foo
     *
     * to this:
     *
     *      li $42, rodata_offset_of(foo)
     *      atom $42
     */
    auto synth = raw;
    synth.operands.pop_back();
    cooked.push_back(emit_instruction(synth));

    std::cerr << "        cooked into " << cooked.size() << " ops\n";

    return cooked;
}
auto expand_double(ast::Instruction const& raw) -> Text
{
    auto cooked = Text{};

    /*
     * Synthesize loading the double offset first. It will emit a
     * sequence of instructions that will load the offset of the
     * data, which will then be used by the DOUBLE instruction to create the
     * object.
     */
    auto li = raw;
    {
        li.leader      = raw.leader;
        li.leader.text = "g.li";
    }
    std::ranges::copy(expand_li(li, true), std::back_inserter(cooked));

    /*
     * Then, synthesize the actual ATOM instruction. This means
     * simply replacing the literal atom (or a reference to one) with the
     * register containing its loaded offset ie, changing this code:
     *
     *      double $42, PI
     *
     * to this:
     *
     *      li $42, rodata_offset_of(PI)
     *      double $42
     */
    auto synth = raw;
    synth.operands.pop_back();
    cooked.push_back(emit_instruction(synth));

    std::cerr << "        cooked into " << cooked.size() << " ops\n";

    return cooked;
}
auto expand_return(ast::Instruction const& raw) -> Text
{
    if (not raw.operands.empty()) {
        return {emit_instruction(raw)};
    }

    using namespace std::string_literals;
    auto synth = raw;

    /*
     * Return instruction takes a single register operand, but this operand can
     * be omitted. As in C, if omitted it defaults to void.
     */
    auto operand = ast::Operand{};
    operand.ingredients.push_back(raw.leader);
    operand.ingredients.back().text  = "void";
    operand.ingredients.back().token = viua::libs::lexer::TOKEN::VOID;
    synth.operands.push_back(operand);

    return {emit_instruction(synth)};
}
auto expand_memory_access(ast::Instruction const& raw) -> Text
{
    using namespace std::string_literals;
    auto synth = ast::Instruction{};

    auto opcode_view = std::string_view{raw.leader.text};
    auto greedy      = opcode_view.starts_with("g.");
    if (greedy) {
        opcode_view.remove_prefix(2);
    }

    synth.leader         = raw.leader;
    synth.leader.text    = opcode_view;
    synth.leader.text[1] = 'm';
    if (opcode_view[0] == 'a') {
        switch (synth.leader.text.back()) {
        case 'a':
            synth.leader.text = "ama";
            break;
        case 'd':
            synth.leader.text = "amd";
            break;
        default:
            abort();
        }
    }

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
        synth.leader.text = ("g." + synth.leader.text);
    }

    return {emit_instruction(synth)};
}
auto expand_immediate_arithmetic(ast::Instruction const& raw) -> Text
{
    auto synth = raw;
    if (synth.operands.back().ingredients.back().text.back() == 'u') {
        synth.leader.text += 'u';
    }
    return {emit_instruction(synth)};
}
auto expand_cast(ast::Instruction const& raw) -> Text
{
    auto synth        = raw;
    auto& target_type = synth.operands.back().ingredients.back();
    if (not FUNDAMENTAL_TYPE_NAMES.contains(target_type.text)) {
        using viua::support::string::levenshtein_filter;
        auto misspell_candidates =
            levenshtein_filter(target_type.text, FUNDAMENTAL_TYPE_NAMES);
        if (misspell_candidates.empty()) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{target_type, Cause::Invalid_cast, target_type.text}.add(
                raw.leader);
        }

        using viua::support::string::levenshtein_best;
        auto best_candidate = levenshtein_best(target_type.text,
                                               misspell_candidates,
                                               (target_type.text.size() / 2));
        if (best_candidate.second == target_type.text) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{target_type, Cause::Invalid_cast, target_type.text}.add(
                raw.leader);
        }

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw did_you_mean(Error{raw.operands.back().ingredients.front(),
                                 Cause::Invalid_cast,
                                 target_type.text},
                           best_candidate.second)
            .add(raw.leader);
    }
    target_type.text = std::to_string(
        static_cast<uintmax_t>(FUNDAMENTAL_TYPE_NAMES.at(target_type.text)));

    return {emit_instruction(synth)};
}

auto expand_instruction(ast::Instruction const& raw,
                        std::vector<Elf64_Sym>& symbol_table,
                        std::map<std::string, size_t> const& symbol_map,
                        Decl_map const& decl_map) -> Text
{
    auto const memory_access = std::set<std::string_view>{
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
    auto const immediate_signed_arithmetic = std::set<std::string_view>{
        "addi",
        "g.addi",
        "subi",
        "g.subi",
        "muli",
        "g.muli",
        "divi",
        "g.divi",
    };

    auto const strip_greedy = [](std::string_view op) -> std::string_view {
        if (op.starts_with("g.")) {
            op.remove_prefix(2);
        }
        return op;
    };
    auto const opcode = strip_greedy(raw.leader.text);

    if (opcode == "li") {
        return expand_li(raw);
    } else if (opcode == "delete") {
        return expand_delete(raw);
    } else if (opcode == "if") {
        return expand_flow_control(raw, symbol_table, symbol_map, decl_map);
    } else if (opcode == "call" or opcode == "actor") {
        return expand_call(raw, symbol_table, symbol_map);
    } else if (opcode == "return") {
        return expand_return(raw);
    } else if (opcode == "atom") {
        return expand_atom(raw);
    } else if (opcode == "double") {
        return expand_double(raw);
    } else if (memory_access.contains(opcode)) {
        return expand_memory_access(raw);
    } else if (opcode == "cast") {
        return expand_cast(raw);
    } else if (immediate_signed_arithmetic.contains(opcode)) {
        return expand_immediate_arithmetic(raw);
    } else {
        return {emit_instruction(raw)};
    }
}

auto cook_instructions(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                       std::vector<uint8_t> const& rodata_buf,
                       std::vector<uint8_t> const& string_table,
                       std::vector<Elf64_Sym>& symbol_table,
                       std::map<std::string, size_t> const& symbol_map,
                       Decl_map const& decl_map) -> Text
{
    enum class SECTION {
        NONE,
        RODATA,
        TEXT,
    };
    auto active_section = SECTION::NONE;
    auto activator      = std::experimental::observer_ptr<ast::Section const>{};

    auto active_function = std::experimental::observer_ptr<Elf64_Sym>{};
    auto function_label  = std::experimental::observer_ptr<ast::Label const>{};

    auto active_symbol = std::experimental::observer_ptr<Elf64_Sym>{};
    auto active_label  = std::string{};
    auto labeller      = std::experimental::observer_ptr<ast::Label const>{};

    auto text = Text{};
    {
        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.emplace_back(
            N{static_cast<instruction_type>(OPCODE::HALT)}.encode());
    }

    (void)rodata_buf;
    (void)string_table;

    auto const save_size_of_active_function =
        [&text, &af = active_function, &function_label]() -> void {
        if (af) {
            af->st_size = ((text.size() * sizeof(viua::arch::instruction_type))
                           - af->st_value);

            std::cerr << "  size of function " << function_label->name.text
                      << " is " << af->st_size << " bytes\n";
            std::cerr << "  span of function " << function_label->name.text
                      << " is "
                      << "[.text+0x" << std::hex << std::setfill('0')
                      << std::setw(16) << af->st_value << "..." << std::setw(16)
                      << (af->st_value + af->st_size
                          - sizeof(viua::arch::instruction_type))
                      << std::dec << std::setfill(' ') << "]\n";
        }
    };

    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;

        if (each->leader.token == TOKEN::SWITCH_TO_SECTION) {
            activator.reset(static_cast<ast::Section const*>(each.get()));
            auto const& sec = *activator;
            auto const name = sec.which();
            if (name == std::string_view{".rodata"}) {
                active_section = SECTION::RODATA;
            } else if (name == std::string_view{".text"}) {
                active_section = SECTION::TEXT;
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{sec.name,
                            Cause::None,
                            "unknown section: " + quote_fancy(name)};
            }
            continue;
        }

        auto const is_label       = (each->leader.token == TOKEN::DEFINE_LABEL);
        auto const is_instruction = (each->leader.token == TOKEN::OPCODE);
        if (auto const is_interesting = is_label or is_instruction;
            not is_interesting) {
            continue;
        }

        if (active_section == SECTION::NONE) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each->leader, Cause::None, "no active section"};
        }
        if (active_section == SECTION::RODATA) {
            continue;
        }

        if (is_label) {
            auto const& lab = static_cast<ast::Label&>(*each);
            labeller.reset(&lab);
            active_label = make_name_from_lexeme(lab.name);

            auto labelled_symbol = std::experimental::observer_ptr<Elf64_Sym>{};
            if (symbol_map.contains(active_label)) {
                labelled_symbol.reset(
                    &symbol_table.at(symbol_map.at(active_label)));
            } else {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{lab.name,
                            Cause::None,
                            "text label without attached symbol"};
            }

            /*
             * The st_value can be set now, because we know for sure that it
             * will contain the right physical address since all the
             * instructions that preceded it were already expanded to their full
             * form.
             */
            labelled_symbol->st_value =
                (text.size() * sizeof(viua::arch::instruction_type));

            auto const is_jump_label =
                (ELF64_ST_BIND(labelled_symbol->st_info) == STB_LOCAL)
                and (labelled_symbol->st_other == STV_HIDDEN);
            auto const is_function_label = not is_jump_label;
            if (is_function_label) {
                save_size_of_active_function();
                active_function = labelled_symbol;
                function_label.reset(&lab);
            }

            std::cerr << (is_jump_label ? "  " : "") << "recording "
                      << (is_jump_label ? "jump" : "call") << " label "
                      << active_label << " at "
                      << "[.text+0x" << std::hex << std::setfill('0')
                      << std::setw(16)
                      << (text.size() * sizeof(viua::arch::instruction_type))
                      << std::dec << std::setfill(' ') << "]\n";
            std::cerr << "      .text has " << text.size() << " op(s)\n";

            active_symbol = labelled_symbol;

            continue;
        }

        if (is_instruction) {
            auto const& raw_instr = static_cast<ast::Instruction&>(*each);
            std::cerr << "    cooking " << raw_instr.leader.text << "\n";

            // 1. expand the raw instruction
            // 2. append the resulting sequence to text
            //
            // The trick is that we do not have to care about physical addresses
            // right now, and thus can expand as we go and disregard the
            // difference between physical and logical index of each individual
            // instruction.
            //
            // Since IF branches are mandaded to point to labels there is no
            // risk of accidentally inserting a physical address at this stage.
            // Just insert the index pointing to the symbol table, and fill it
            // in in the next step.

            std::ranges::copy(
                expand_instruction(
                    raw_instr, symbol_table, symbol_map, decl_map),
                std::back_inserter(text));
            std::cerr << "      .text has " << text.size() << " op(s)\n";
        }
    }
    save_size_of_active_function();

    {
        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.emplace_back(
            N{static_cast<instruction_type>(OPCODE::HALT)}.encode());
    }

    return text;
}

auto find_entry_point(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                      std::vector<Elf64_Sym>& symbol_table,
                      std::map<std::string, size_t>& symbol_map)
    -> std::optional<viua::libs::lexer::Lexeme>
{
    auto entry_point_fn = std::optional<viua::libs::lexer::Lexeme>{};
    for (auto const& each : nodes) {
        using viua::libs::lexer::TOKEN;
        auto const is_symbol = (each->leader.token == TOKEN::DECLARE_SYMBOL);
        auto const is_entry_point = each->has_attr("entry_point");

        if (is_entry_point and not is_symbol) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            throw Error{each->leader,
                        Cause::None,
                        "only .symbol can be tagged as entry point"}
                .add(*each->attr("entry_point"));
        }

        if (is_entry_point) {
            if (entry_point_fn.has_value()) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto const dup = static_cast<ast::Symbol const&>(*each);
                throw Error{
                    dup.name, Cause::Duplicated_entry_point, dup.name.text}
                    .add(dup.attr("entry_point").value())
                    .note("first entry point was: " + entry_point_fn->text);
            }
            entry_point_fn = static_cast<ast::Symbol const&>(*each).name;

            auto const sym =
                symbol_table.at(symbol_map.at(entry_point_fn->text));
            auto const not_global  = (ELF64_ST_BIND(sym.st_info) != STB_GLOBAL);
            auto const not_visible = (sym.st_other != STV_DEFAULT);
            auto const not_function = (ELF64_ST_TYPE(sym.st_info) != STT_FUNC);
            if (not_function) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                throw Error{*entry_point_fn,
                            Cause::None,
                            "entry point must be a function"}
                    .add(*each->attr("entry_point"));
            }
            if (not_global) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{*each->attr("local"),
                               Cause::None,
                               "entry point must be global"};
                e.add(*each->attr("entry_point"));
                e.add(*entry_point_fn, true);
                e.aside("remove the " + quote_fancy("local")
                        + " tag, or change it to explicit "
                        + quote_fancy("global"));
                throw e;
            }
            if (not_visible) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto e = Error{*each->attr("hidden"),
                               Cause::None,
                               "entry point must not be hidden"};
                e.add(*each->attr("entry_point"));
                e.add(*entry_point_fn, true);
                e.aside("remove the " + quote_fancy("hidden") + " tag");
                throw e;
            }
        }
    }
    return entry_point_fn;
}

auto make_reloc_table(Text const& text) -> std::vector<Elf64_Rel>
{
    auto reloc_table = std::vector<Elf64_Rel>{};

    auto const push_reloc = [&text, &reloc_table](size_t const i) -> void {
        using viua::arch::ops::OPCODE;
        auto const op =
            static_cast<OPCODE>(text.at(i) & viua::arch::ops::OPCODE_MASK);

        using enum viua::arch::elf::R_VIUA;
        auto const into_rodata = (op == OPCODE::ATOM) or (op == OPCODE::DOUBLE)
                                 or (op == OPCODE::ARODP);
        auto const type = into_rodata ? R_VIUA_OBJECT : R_VIUA_JUMP_SLOT;

        auto symtab_entry_index = uint32_t{};
        if (op == OPCODE::ARODP) {
            using viua::arch::ops::E;
            symtab_entry_index =
                static_cast<uint32_t>(E::decode(text.at(i)).immediate);
        } else {
            using viua::arch::ops::F;
            auto const hi =
                static_cast<uint64_t>(F::decode(text.at(i - 2)).immediate)
                << 32;
            auto const lo      = F::decode(text.at(i - 1)).immediate;
            symtab_entry_index = static_cast<uint32_t>(hi | lo);
        }

        Elf64_Rel rel;
        rel.r_offset = i * sizeof(viua::arch::instruction_type);
        rel.r_info =
            ELF64_R_INFO(symtab_entry_index, static_cast<uint8_t>(type));
        reloc_table.push_back(rel);
    };

    for (auto i = size_t{0}; i < text.size(); ++i) {
        using viua::arch::opcode_type;
        using viua::arch::ops::OPCODE;

        auto const each = text.at(i);

        auto const op =
            static_cast<OPCODE>(each & viua::arch::ops::OPCODE_MASK);
        switch (op) {
            using enum viua::arch::ops::OPCODE;
        case IF:
        case CALL:
        case ATOM:
        case DOUBLE:
        case ARODP:
            push_reloc(i);
            break;
        default:
            break;
        }
    }

    return reloc_table;
}

auto emit_elf(std::filesystem::path const output_path,
              bool const as_executable,
              std::optional<uint64_t> const entry_point_fn,
              Text const& text,
              std::optional<std::vector<Elf64_Rel>> relocs,
              std::vector<uint8_t> const& rodata_buf,
              std::vector<uint8_t> const& string_table,
              std::vector<Elf64_Sym>& symbol_table) -> void
{
    auto const a_out = open(output_path.c_str(),
                            O_CREAT | O_TRUNC | O_WRONLY,
                            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (a_out == -1) {
        close(a_out);
        exit(1);
    }

    constexpr auto VIUA_MAGIC [[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
    auto const VIUAVM_INTERP                   = std::string{"viua-vm"};
    auto const VIUA_COMMENT = std::string{VIUAVM_VERSION_FULL};

    {
        // see elf(5)
        Elf64_Ehdr elf_header{};
        elf_header.e_ident[EI_MAG0]       = '\x7f';
        elf_header.e_ident[EI_MAG1]       = 'E';
        elf_header.e_ident[EI_MAG2]       = 'L';
        elf_header.e_ident[EI_MAG3]       = 'F';
        elf_header.e_ident[EI_CLASS]      = ELFCLASS64;
        elf_header.e_ident[EI_DATA]       = ELFDATA2LSB;
        elf_header.e_ident[EI_VERSION]    = EV_CURRENT;
        elf_header.e_ident[EI_OSABI]      = ELFOSABI_STANDALONE;
        elf_header.e_ident[EI_ABIVERSION] = 0;
        elf_header.e_type                 = (as_executable ? ET_EXEC : ET_REL);
        elf_header.e_machine              = ET_NONE;
        elf_header.e_version              = elf_header.e_ident[EI_VERSION];
        elf_header.e_flags  = 0;  // processor-specific flags, should be 0
        elf_header.e_ehsize = sizeof(elf_header);

        auto shstr            = std::vector<char>{'\0'};
        auto save_shstr_entry = [&shstr](std::string_view const sv) -> size_t {
            auto const saved_at = shstr.size();
            std::copy(sv.begin(), sv.end(), std::back_inserter(shstr));
            shstr.push_back('\0');
            return saved_at;
        };

        auto text_section_ndx   = size_t{0};
        auto rel_section_ndx    = size_t{0};
        auto rodata_section_ndx = size_t{0};
        auto symtab_section_ndx = size_t{0};
        auto strtab_section_ndx = size_t{0};

        using Header_pair = std::pair<std::optional<Elf64_Phdr>, Elf64_Shdr>;
        auto elf_headers  = std::vector<Header_pair>{};

        {
            /*
             * It is mandated by ELF that the first section header is void, and
             * must be all zeroes. It is reserved and used by ELF extensions.
             *
             * We do not extend ELF in any way, so this section is SHT_NULL for
             * Viua VM.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_NULL;
            seg.p_offset = 0;
            seg.p_filesz = 0;

            Elf64_Shdr void_section{};
            void_section.sh_type = SHT_NULL;

            elf_headers.push_back({seg, void_section});
        }
        {
            /*
             * .viua.magic
             *
             * The second section (and the first fragment) is the magic number
             * Viua uses to detect the if the binary *really* is something it
             * can handle, and on Linux by the binfmt.d(5) to enable running
             * Viua ELFs automatically.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_NULL;
            seg.p_offset = 0;
            memcpy(&seg.p_offset, VIUA_MAGIC, 8);
            seg.p_filesz = 8;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".viua.magic");
            sec.sh_type   = SHT_NOBITS;
            sec.sh_offset = sizeof(Elf64_Ehdr) + offsetof(Elf64_Phdr, p_offset);
            sec.sh_size   = 8;
            sec.sh_flags  = 0;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .interp
             *
             * What follows is the interpreter. This is mostly useful to get
             * better reporting out of readelf(1) and file(1). It also serves as
             * a second thing to check for if the file *really* is a Viua
             * binary.
             */

            Elf64_Phdr seg{};
            seg.p_type   = PT_INTERP;
            seg.p_offset = 0;
            seg.p_filesz = VIUAVM_INTERP.size() + 1;
            seg.p_flags  = PF_R;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".interp");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUAVM_INTERP.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({seg, sec});
        }
        if (relocs.has_value()) {
            /*
             * .rel
             */
            auto const relocation_table = *relocs;

            Elf64_Shdr sec{};
            sec.sh_name    = save_shstr_entry(".rel");
            sec.sh_type    = SHT_REL;
            sec.sh_offset  = 0;
            sec.sh_entsize = sizeof(decltype(relocation_table)::value_type);
            sec.sh_size    = (relocation_table.size() * sec.sh_entsize);
            sec.sh_flags   = SHF_INFO_LINK;

            /*
             * This should point to .symtab section that is relevant for the
             * relocations contained in this .rel section (in our case its the
             * only .symtab section in the ELF), but we do not know that
             * section's index yet.
             */
            sec.sh_link = 0;

            /*
             * This should point to .text section (or any other section) to
             * which the relocations apply. We do not know that index yet, but
             * it MUST be patched later.
             */
            sec.sh_info = 0;

            rel_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .text
             *
             * The first segment and section pair that contains something users
             * of Viua can affect is the .text section ie, the executable
             * instructions representing user programs.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_LOAD;
            seg.p_offset = 0;
            auto const sz =
                (text.size()
                 * sizeof(std::decay_t<decltype(text)>::value_type));
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R | PF_X;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".text");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC | SHF_EXECINSTR;

            text_section_ndx = elf_headers.size();
            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .rodata
             *
             * Then, the .rodata section containing user data. Only constants
             * are allowed to be defined as data labels in Viua -- there are no
             * global variables.
             *
             * The "strings table" contains not only strings but also floats,
             * atoms, and any other piece of data that does not fit into a
             * single load instruction (with the exception of long integers
             * which are loaded using a sequence of raw instructions - this
             * allows loading addresses, which are then used to index strings
             * table).
             */
            Elf64_Phdr seg{};
            seg.p_type    = PT_LOAD;
            seg.p_offset  = 0;
            auto const sz = rodata_buf.size();
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".rodata");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC;

            rodata_section_ndx = elf_headers.size();
            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .comment
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".comment");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUA_COMMENT.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .symtab
             *
             * Last, but not least, comes another LOAD segment.
             * It contains a symbol table with function addresses.
             *
             * Function calls use this table to determine the address to which
             * they should transfer control - there are no direct calls.
             * Inefficient, but flexible.
             */
            Elf64_Shdr sec{};
            sec.sh_name = save_shstr_entry(".symtab");
            /*
             * This could be SHT_SYMTAB, but the SHT_SYMTAB type sections expect
             * a certain format of the symbol table which Viua does not use. So
             * let's just use SHT_PROGBITS because interpretation of
             * SHT_PROGBITS is up to the program.
             */
            sec.sh_type    = SHT_SYMTAB;
            sec.sh_offset  = 0;
            sec.sh_size    = (symbol_table.size() * sizeof(Elf64_Sym));
            sec.sh_flags   = 0;
            sec.sh_entsize = sizeof(Elf64_Sym);
            sec.sh_info    = 0;

            symtab_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .strtab
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".strtab");
            sec.sh_type   = SHT_STRTAB;
            sec.sh_offset = 0;
            sec.sh_size   = string_table.size();
            sec.sh_flags  = SHF_STRINGS;

            strtab_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .shstrtab
             *
             * ACHTUNG! ATTENTION! UWAGA! POZOR! TÄHELEPANU!
             *
             * This section contains the strings table representing section
             * names. If any more sections are added they MUST APPEAR BEFORE
             * THIS SECTION. Otherwise the strings won't be available because
             * the size of the section will not be correct and will appear as
             * <corrupt> in readelf(1) output.
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".shstrtab");
            sec.sh_type   = SHT_STRTAB;
            sec.sh_offset = 0;
            sec.sh_size   = shstr.size();
            sec.sh_flags  = SHF_STRINGS;

            elf_headers.push_back({std::nullopt, sec});
        }

        /*
         * Link the .symtab to its associated .strtab; otherwise you will
         * get <corrupt> names when invoking readelf(1) to inspect the file.
         */
        elf_headers.at(symtab_section_ndx).second.sh_link = strtab_section_ndx;

        /*
         * Patch the symbol table section index.
         */
        if (relocs.has_value()) {
            elf_headers.at(rel_section_ndx).second.sh_link = symtab_section_ndx;
            elf_headers.at(rel_section_ndx).second.sh_info = text_section_ndx;
        }

        auto elf_pheaders = std::count_if(
            elf_headers.begin(),
            elf_headers.end(),
            [](auto const& each) -> bool { return each.first.has_value(); });
        auto elf_sheaders = elf_headers.size();

        auto const elf_size = sizeof(Elf64_Ehdr)
                              + (elf_pheaders * sizeof(Elf64_Phdr))
                              + (elf_sheaders * sizeof(Elf64_Shdr));
        auto text_offset = std::optional<size_t>{};
        {
            auto offset_accumulator = size_t{0};
            for (auto& [segment, section] : elf_headers) {
                if (segment.has_value() and (segment->p_type != PT_NULL)) {
                    if (segment->p_type == PT_NULL) {
                        continue;
                    }

                    /*
                     * The thing that Viua VM mandates is that the main function
                     * (if it exists) MUST be put in the first executable
                     * segment. This can be elegantly achieved by blindly
                     * pushing the address of first such segment.
                     *
                     * The following construction using std::optional:
                     *
                     *      x = x.value_or(y)
                     *
                     * ensures that x will store the first assigned value
                     * without any checks. Why not use somethin more C-like? For
                     * example:
                     *
                     *      x = (x ? x : y)
                     *
                     * looks like it achieves the same without any fancy-shmancy
                     * types. Yeah, it only looks like it does so. If the first
                     * executable segment would happen to be at offset 0 then
                     * the C-style code fails, while the C++-style is correct.
                     * As an aside: this ie, C style being broken an C++ being
                     * correct is something surprisingly common. Or rather more
                     * functional style being correct... But I digress.
                     */
                    if (segment->p_flags == (PF_R | PF_X)) {
                        text_offset = text_offset.value_or(offset_accumulator);
                    }

                    segment->p_offset = (elf_size + offset_accumulator);
                }

                if (section.sh_type == SHT_NULL) {
                    continue;
                }
                if (section.sh_type == SHT_NOBITS) {
                    continue;
                }

                section.sh_offset = (elf_size + offset_accumulator);
                offset_accumulator += section.sh_size;
            }
        }

        elf_header.e_entry = entry_point_fn.has_value()
                                 ? (*text_offset + *entry_point_fn + elf_size)
                                 : 0;

        elf_header.e_phoff     = sizeof(Elf64_Ehdr);
        elf_header.e_phentsize = sizeof(Elf64_Phdr);
        elf_header.e_phnum     = static_cast<Elf64_Half>(elf_pheaders);

        elf_header.e_shoff =
            elf_header.e_phoff + (elf_pheaders * sizeof(Elf64_Phdr));
        elf_header.e_shentsize = sizeof(Elf64_Shdr);
        elf_header.e_shnum     = static_cast<Elf64_Half>(elf_sheaders);
        elf_header.e_shstrndx  = static_cast<Elf64_Half>(elf_sheaders - 1);

        viua::support::posix::whole_write(
            a_out, &elf_header, sizeof(elf_header));

        /*
         * Unfortunately, we have to have use two loops here because segment and
         * section headers cannot be interweaved. We could do some lseek(2)
         * tricks, but I don't think it's worth it. For-each loops are simple
         * and do not require any special bookkeeping to work correctly.
         */
        for (auto const& [segment, _] : elf_headers) {
            if (not segment) {
                continue;
            }
            viua::support::posix::whole_write(
                a_out,
                &*segment,
                sizeof(std::remove_reference_t<decltype(*segment)>));
        }
        for (auto const& [_, section] : elf_headers) {
            viua::support::posix::whole_write(
                a_out,
                &section,
                sizeof(std::remove_reference_t<decltype(section)>));
        }

        viua::support::posix::whole_write(
            a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

        if (relocs.has_value()) {
            for (auto const& rel : *relocs) {
                viua::support::posix::whole_write(
                    a_out, &rel, sizeof(std::decay_t<decltype(rel)>));
            }
        }

        auto const text_size =
            (text.size() * sizeof(std::decay_t<decltype(text)>::value_type));
        viua::support::posix::whole_write(a_out, text.data(), text_size);

        viua::support::posix::whole_write(
            a_out, rodata_buf.data(), rodata_buf.size());

        viua::support::posix::whole_write(
            a_out, VIUA_COMMENT.c_str(), VIUA_COMMENT.size() + 1);

        for (auto& each : symbol_table) {
            switch (ELF64_ST_TYPE(each.st_info)) {
            case STT_FUNC:
                each.st_shndx = static_cast<Elf64_Section>(text_section_ndx);
                break;
            case STT_OBJECT:
                each.st_shndx = static_cast<Elf64_Section>(rodata_section_ndx);
                break;
            default:
                break;
            }
            viua::support::posix::whole_write(
                a_out, &each, sizeof(std::decay_t<decltype(symbol_table)>));
        }

        viua::support::posix::whole_write(
            a_out, string_table.data(), string_table.size());

        viua::support::posix::whole_write(a_out, shstr.data(), shstr.size());
    }

    close(a_out);
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    auto const args = std::vector<std::string>{(argv + 1), (argv + argc)};
    if (args.empty()) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": no file to assemble\n";
        return 1;
    }

    auto preferred_output_path = std::optional<std::filesystem::path>{};
    auto verbosity_level       = 0;
    auto show_version          = false;
    auto show_help             = false;

    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "--") {
            // explicit separator of options and operands
            ++i;
            break;
        }
        /*
         * Tool-specific options.
         */
        else if (each == "-o") {
            preferred_output_path = std::filesystem::path{args.at(++i)};
        }
        /*
         * Common options.
         */
        else if (each == "-v" or each == "--verbose") {
            ++verbosity_level;
        } else if (each == "--version") {
            show_version = true;
        } else if (each == "--help") {
            show_help = true;
        } else if (each.front() == '-') {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": unknown option: " << each << "\n";
            return 1;
        } else {
            // input files start here
            ++i;
            break;
        }
    }

    if (show_version) {
        if (verbosity_level) {
            std::cout << "Viua VM ";
        }
        std::cout << (verbosity_level ? VIUAVM_VERSION_FULL : VIUAVM_VERSION)
                  << "\n";
        return 0;
    }
    if (show_help) {
        if (execlp("man", "man", "1", "viua-asm", nullptr) == -1) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": man(1) page not installed or not found\n";
            return 1;
        }
    }

    /*
     * If invoked *with* some arguments, find the path to the source file and
     * assemble it - converting assembly source code into binary. Produced
     * binary may be:
     *
     *  - executable (default): an ELF executable, suitable to be run by Viua VM
     *    kernel
     *  - linkable (with -c flag): an ELF relocatable object file, which should
     *    be linked with other object files to produce a final executable or
     *    shared object
     */
    auto const source_path = std::filesystem::path{args.back()};
    auto source_text       = std::string{};
    {
        auto const source_fd = open(source_path.c_str(), O_RDONLY);
        if (source_fd == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = viua::support::errno_desc(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }

        struct stat source_stat {};
        if (fstat(source_fd, &source_stat) == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = viua::support::errno_desc(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }
        if (source_stat.st_size == 0) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": "
                      << esc(2, COLOR_FG_ORANGE_RED_1) << "warning"
                      << esc(2, ATTR_RESET) << ": empty source file\n";
        }

        source_text.resize(source_stat.st_size);
        viua::support::posix::whole_read(
            source_fd, source_text.data(), source_text.size());
        close(source_fd);
    }

    auto const output_path = preferred_output_path.value_or(
        [source_path]() -> std::filesystem::path {
            auto o = source_path;
            o.replace_extension("o");
            return o;
        }());

    /*
     * Lexical analysis (lexing).
     *
     * Split the loaded source code into a stream of lexemes for easier
     * processing later. The first point at which errors are detected eg, if
     * illegal characters are used, strings are unclosed, etc.
     */
    auto lexemes = viua::libs::lexer::stage::lex(source_path, source_text);
    if constexpr (false and DEBUG_LEX) {
        std::cerr << lexemes.size() << " raw lexeme(s)\n";
        dump_lexemes(lexemes);
    }

    lexemes = viua::libs::lexer::stage::remove_noise(std::move(lexemes));
    if constexpr (DEBUG_LEX) {
        std::cerr << lexemes.size() << " cooked lexeme(s)\n";
        dump_lexemes(lexemes);
    }

    if (auto e = any_find_mistake(lexemes); e.has_value()) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, *e);
    }

    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};
    try {
        nodes = parse(lexemes);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
    if constexpr (DEBUG_PARSE) {
        std::cerr << nodes.size() << " AST nodes(s)\n";
        dump_nodes(nodes);
    }

    auto declared_symbols = Decl_map{};
    auto rodata_contents  = std::vector<uint8_t>{};
    auto string_table     = std::vector<uint8_t>{};
    auto symbol_table     = std::vector<Elf64_Sym>{};
    auto symbol_map       = std::map<std::string, size_t>{};
    auto fn_offsets       = std::map<std::string, size_t>{};

    /*
     * Allocate the first 64 bits of .rodata to prevent any local values from
     * having address of 0.
     */
    rodata_contents.resize(8);

    /*
     * ELF standard requires the first byte in the string table to be zero.
     */
    string_table.push_back('\0');

    {
        auto empty     = Elf64_Sym{};
        empty.st_name  = STN_UNDEF;
        empty.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
        empty.st_shndx = SHN_UNDEF;
        symbol_table.push_back(empty);

        auto file_sym = Elf64_Sym{};
        file_sym.st_name =
            save_string_to_strtab(string_table, source_path.native());
        file_sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_FILE);
        file_sym.st_shndx = SHN_ABS;
        symbol_table.push_back(file_sym);
    }

    try {
        save_declared_symbols(
            nodes, string_table, symbol_table, symbol_map, declared_symbols);
        cache_function_labels(nodes, string_table, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

    try {
        save_objects(nodes,
                     rodata_contents,
                     string_table,
                     symbol_table,
                     symbol_map,
                     declared_symbols);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

    auto text = Text{};
    try {
        text = cook_instructions(nodes,
                                 rodata_contents,
                                 string_table,
                                 symbol_table,
                                 symbol_map,
                                 declared_symbols);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }

    /*
     * ELF standard requires the last byte in the string table to be zero.
     */
    string_table.push_back('\0');

    /*
     * Detect entry point function.
     *
     * We're not handling relocatable files (shared libs, etc) yet so it makes
     * sense to enforce entry function presence in all cases. Once the
     * relocatables and separate compilation is supported again, this should be
     * hidden behind a flag.
     */
    auto entry_point_fn = std::optional<viua::libs::lexer::Lexeme>{};
    try {
        entry_point_fn = find_entry_point(nodes, symbol_table, symbol_map);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
    }
    if (entry_point_fn.has_value()) {
        auto const sym = symbol_table.at(symbol_map.at(entry_point_fn->text));
        std::cerr << "entry point is " << entry_point_fn->text
                  << " at [.text+0x" << std::hex << std::setfill('0')
                  << std::setw(16) << sym.st_value << std::dec
                  << std::setfill(' ') << "]\n";
    }

    for (auto const& [decl_sym, decl_sec] : declared_symbols) {
        auto const& sym = symbol_table.at(
            symbol_map.at(make_name_from_lexeme(decl_sym.name)));

        auto const was_declared_extern =
            static_cast<bool>(decl_sym.has_attr("extern"));
        auto const was_defined = sym.st_value and sym.st_size;

        if (was_declared_extern and was_defined) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto e = Error{
                decl_sym.name,
                Cause::None,
                "symbol declared [[extern]], but a definition was provided"};
            viua::libs::stage::display_error_and_exit(
                source_path, source_text, e);
        } else if ((not was_declared_extern) and (not was_defined)) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;

            auto e = Error{decl_sym.name,
                           Cause::None,
                           "symbol declared, but no definition was provided"};
            e.note("add [[extern]] if the symbol is defined in another "
                   "compilation unit or module");
            viua::libs::stage::display_error_and_exit(
                source_path, source_text, e);
        }
    }

    /*
     * ELF emission.
     */
    auto const reloc_table = make_reloc_table(text);
    emit_elf(
        output_path,
        false,
        (entry_point_fn.has_value()
             ? std::optional{symbol_table
                                 .at(symbol_map.at(entry_point_fn.value().text))
                                 .st_value}
             : std::nullopt),
        text,
        reloc_table,
        rodata_contents,
        string_table,
        symbol_table);

    return 0;
}
