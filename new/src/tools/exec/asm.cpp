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

#include <elf.h>
#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>
#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/support/vector.h>


constexpr auto DEBUG_ELF       = false;
constexpr auto DEBUG_LEX       = false;
constexpr auto DEBUG_EXPANSION = false;


namespace {
auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint64_t, std::pair<std::pair<uint32_t, uint32_t>, uint32_t>>
{
    constexpr auto LOW_24  = uint64_t{0x0000000000ffffff};
    constexpr auto HIGH_36 = uint64_t{0xfffffffff0000000};

    auto const high_part = ((value & HIGH_36) >> 28);
    auto const low_part  = static_cast<uint32_t>(value & ~HIGH_36);

    /*
     * If the low part consists of only 24 bits we can use just two
     * instructions:
     *
     *  1/ lui to load high 36 bits
     *  2/ addi to add low 24 bits
     *
     * This reduces the overhead of loading 64-bit values.
     */
    if ((low_part & LOW_24) == low_part) {
        return {high_part, {{low_part, 0}, 0}};
    }

    auto const multiplier = 16;
    auto const remainder  = (low_part % multiplier);
    auto const base       = (low_part - remainder) / multiplier;

    return {high_part, {{base, multiplier}, remainder}};
}
auto li_cost(uint64_t const value) -> size_t
{
    /*
     * Remember to keep this function in sync with what expand_li() function
     * does. If this function ie, li_cost() returns a bad value, or expand_li()
     * starts emitting different instructions -- branch calculations will fail.
     */
    auto count = size_t{0};

    auto parts = to_loading_parts_unsigned(value);
    if (parts.first) {
        ++count;  // lui
    }

    auto const multiplier = parts.second.first.second;
    if (multiplier != 0) {
        ++count;  // g.addiu
        ++count;  // g.addiu
        ++count;  // g.mul
        ++count;  // g.addiu
        ++count;  // g.add
        ++count;  // g.add
        ++count;  // g.delete
        ++count;  // g.delete
    } else {
        ++count;  // addiu
    }

    return count;
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
auto save_fn_address(std::vector<uint8_t>& strings, std::string_view const fn)
    -> size_t
{
    auto const fn_size = htole64(static_cast<uint64_t>(fn.size()));
    strings.resize(strings.size() + sizeof(fn_size));
    memcpy((strings.data() + strings.size() - sizeof(fn_size)),
           &fn_size,
           sizeof(fn_size));

    auto const saved_location = strings.size();
    std::copy(fn.begin(), fn.end(), std::back_inserter(strings));

    auto const fn_addr = uint64_t{0};
    strings.resize(strings.size() + sizeof(fn_addr));
    memcpy((strings.data() + strings.size() - sizeof(fn_addr)),
           &fn_addr,
           sizeof(fn_addr));

    return saved_location;
}
auto patch_fn_address(std::vector<uint8_t>& strings,
                      size_t const fn_offset,
                      uint64_t fn_addr) -> void
{
    auto fn_size = uint64_t{};
    memcpy(&fn_size,
           (strings.data() + fn_offset - sizeof(fn_size)),
           sizeof(fn_size));
    fn_size = le64toh(fn_size);

    fn_addr = htole64(fn_addr);
    memcpy((strings.data() + fn_offset + fn_size), &fn_addr, sizeof(fn_addr));
}

auto did_you_mean(viua::libs::errors::compile_time::Error& e, std::string what)
    -> viua::libs::errors::compile_time::Error&
{
    return e.aside("did you mean \"" + what + "\"?");
}
auto did_you_mean(viua::libs::errors::compile_time::Error&& e, std::string what)
    -> viua::libs::errors::compile_time::Error
{
    did_you_mean(e, what);
    return e;
}
}  // anonymous namespace

namespace ast {
struct Node {
    using Lexeme         = viua::libs::lexer::Lexeme;
    using attribute_type = std::pair<Lexeme, std::optional<Lexeme>>;
    std::vector<attribute_type> attributes;

    auto has_attr(std::string_view const) const -> bool;
    auto attr(std::string_view const) const -> std::optional<Lexeme>;

    virtual auto to_string() const -> std::string = 0;
    virtual ~Node()                               = default;
};
auto Node::has_attr(std::string_view const key) const -> bool
{
    for (auto const& each : attributes) {
        if (each.first == key) {
            return true;
        }
    }
    return false;
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

struct Operand : Node {
    std::vector<viua::libs::lexer::Lexeme> ingredients;

    auto to_string() const -> std::string override;

    auto make_access() const -> viua::arch::Register_access;

    virtual ~Operand() = default;
};
auto Operand::to_string() const -> std::string
{
    auto out = std::ostringstream{};
    for (auto const& each : ingredients) {
        out << each.text;
    }
    return out.str();
}
auto Operand::make_access() const -> viua::arch::Register_access
{
    if (ingredients.front() == "void") {
        return viua::arch::Register_access{};
    }

    using viua::libs::lexer::TOKEN;
    auto const lx = ingredients.front();
    if (lx != TOKEN::RA_DIRECT and lx != TOKEN::RA_PTR_DEREF) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{ingredients.front(), Cause::Invalid_register_access};
    }

    auto const direct = (lx == TOKEN::RA_DIRECT);
    auto const index  = std::stoul(ingredients.at(1).text);
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
            .note("valid register set specifiers are 'l', 'a', and 'p'");
    }
}

struct Instruction : Node {
    viua::libs::lexer::Lexeme opcode;
    std::vector<Operand> operands;

    size_t physical_index{static_cast<size_t>(-1)};

    auto to_string() const -> std::string override;
    auto parse_opcode() const -> viua::arch::opcode_type;

    virtual ~Instruction() = default;
};
auto Instruction::to_string() const -> std::string
{
    auto out = std::ostringstream{};
    out << opcode.text;
    for (auto const& each : operands) {
        out << ' ' << each.to_string() << ',';
    }

    auto s = out.str();
    if (not operands.empty()) {
        /*
         * If the instruction had any operands then the string will end with a
         * comma (to make the formatting code for operands simpler). We don't
         * want this because this would emit syntactically incorrect assembly
         * code.
         *
         * Thus, if there were any operands - we just remove the final comma.
         */
        s = s.erase(s.rfind(','));
    }
    return s;
}
auto Instruction::parse_opcode() const -> viua::arch::opcode_type
{
    return viua::arch::ops::parse_opcode(opcode.text);
}

struct Fn_def : Node {
    viua::libs::lexer::Lexeme name;
    std::vector<Instruction> instructions;

    viua::libs::lexer::Lexeme start;
    viua::libs::lexer::Lexeme end;

    auto to_string() const -> std::string override;

    virtual ~Fn_def() = default;
};
auto Fn_def::to_string() const -> std::string
{
    return viua::libs::lexer::to_string(viua::libs::lexer::TOKEN::DEF_FUNCTION)
           + ' ' + std::to_string(name.location.line + 1) + ':'
           + std::to_string(name.location.character + 1) + '-'
           + std::to_string(name.location.character + name.text.size()) + ' '
           + name.text;
}

struct Label_def : Node {
    viua::libs::lexer::Lexeme name;
    viua::libs::lexer::Lexeme type;
    std::vector<viua::libs::lexer::Lexeme> value;

    viua::libs::lexer::Lexeme start;
    viua::libs::lexer::Lexeme end;

    auto to_string() const -> std::string override;

    virtual ~Label_def() = default;
};
auto Label_def::to_string() const -> std::string
{
    return viua::libs::lexer::to_string(viua::libs::lexer::TOKEN::DEF_LABEL)
           + ' ' + std::to_string(name.location.line + 1) + ':'
           + std::to_string(name.location.character + 1) + '-'
           + std::to_string(name.location.character + name.text.size()) + ' '
           + name.text;
}

auto remove_noise(std::vector<viua::libs::lexer::Lexeme> raw)
    -> std::vector<viua::libs::lexer::Lexeme>
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
}  // namespace ast

namespace {
auto consume_token_of(
    viua::libs::lexer::TOKEN const tt,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (lexemes.front().token != tt) {
        throw lexemes.front();
    }
    auto lx = std::move(lexemes.front());
    lexemes.remove_prefix(1);
    return lx;
}
auto consume_token_of(
    std::set<viua::libs::lexer::TOKEN> const ts,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (ts.count(lexemes.front().token) == 0) {
        throw lexemes.front();
    }
    auto lx = std::move(lexemes.front());
    lexemes.remove_prefix(1);
    return lx;
}

auto look_ahead(
    viua::libs::lexer::TOKEN const tk,
    viua::support::vector_view<viua::libs::lexer::Lexeme> const& lexemes)
    -> bool
{
    return (not lexemes.empty()) and (lexemes.front() == tk);
}
auto look_ahead(
    std::set<viua::libs::lexer::TOKEN> const ts,
    viua::support::vector_view<viua::libs::lexer::Lexeme> const& lexemes)
    -> bool
{
    return (not lexemes.empty()) and (ts.count(lexemes.front().token) != 0);
}

auto parse_attr_list(
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> std::vector<ast::Node::attribute_type>
{
    auto attrs = std::vector<ast::Node::attribute_type>{};

    using viua::libs::lexer::TOKEN;
    consume_token_of(TOKEN::ATTR_LIST_OPEN, lexemes);
    while ((not lexemes.empty())
           and lexemes.front() != TOKEN::ATTR_LIST_CLOSE) {
        auto key   = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
        auto value = viua::libs::lexer::Lexeme{};

        if (look_ahead(TOKEN::EQ, lexemes)) {
            consume_token_of(TOKEN::EQ, lexemes);

            if (look_ahead(TOKEN::LITERAL_INTEGER, lexemes)) {
                value = consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
            } else if (look_ahead(TOKEN::LITERAL_STRING, lexemes)) {
                value = consume_token_of(TOKEN::LITERAL_STRING, lexemes);
            } else if (look_ahead(TOKEN::LITERAL_ATOM, lexemes)) {
                value = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
            } else {
                throw lexemes.front();
            }
        } else {
            /*
             * If the attribute does not have a value, use the key as the value.
             * This way we never have "valueless" attributes.
             */
            value = key;
        }

        attrs.emplace_back(std::move(key), std::move(value));
    }
    consume_token_of(TOKEN::ATTR_LIST_CLOSE, lexemes);

    return attrs;
}
auto parse_function_definition(
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> std::unique_ptr<ast::Node>
{
    auto fn = std::make_unique<ast::Fn_def>();

    using viua::libs::lexer::TOKEN;
    fn->start = consume_token_of(TOKEN::DEF_FUNCTION, lexemes);

    if (look_ahead(TOKEN::ATTR_LIST_OPEN, lexemes)) {
        fn->attributes = parse_attr_list(lexemes);
    }

    auto fn_name =
        consume_token_of({TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);

    auto instructions       = std::vector<std::unique_ptr<ast::Node>>{};
    auto ins_physical_index = size_t{0};
    while ((not lexemes.empty()) and lexemes.front() != TOKEN::END) {
        auto instruction = ast::Instruction{};

        try {
            instruction.opcode = consume_token_of(TOKEN::OPCODE, lexemes);
        } catch (viua::libs::lexer::Lexeme const& e) {
            if (e.token != viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                throw;
            }

            using viua::libs::lexer::OPCODE_NAMES;
            using viua::support::string::levenshtein_filter;
            auto misspell_candidates = levenshtein_filter(e.text, OPCODE_NAMES);
            if (misspell_candidates.empty()) {
                throw;
            }

            using viua::support::string::levenshtein_best;
            auto best_candidate = levenshtein_best(
                e.text, misspell_candidates, (e.text.size() / 2));
            if (best_candidate.second == e.text) {
                throw;
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
            fn->instructions.push_back(std::move(instruction));
            fn->instructions.back().physical_index = ins_physical_index++;
            continue;
        }

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
                operand.attributes = parse_attr_list(lexemes);
            }

            /*
             * Consume the operand: void, register access, a literal value. This
             * will supply some value for the instruction to work on. This chain
             * of if-else should handle valid operands - and ONLY operands, not
             * their separators.
             */
            if (lexemes.front() == TOKEN::RA_VOID) {
                operand.ingredients.push_back(
                    consume_token_of(TOKEN::RA_VOID, lexemes));
            } else if (look_ahead({TOKEN::RA_DIRECT, TOKEN::RA_PTR_DEREF},
                                  lexemes)) {
                auto const access = consume_token_of(
                    {TOKEN::RA_DIRECT, TOKEN::RA_PTR_DEREF}, lexemes);
                auto index = viua::libs::lexer::Lexeme{};
                try {
                    index = consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
                } catch (viua::libs::lexer::Lexeme const& e) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{e, Cause::Invalid_register_access}
                        .add(access)
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
                    throw Error{index, Cause::Invalid_register_access}
                        .add(access)
                        .aside(
                            "register index range is 0-"
                            + std::to_string(viua::arch::MAX_REGISTER_INDEX));
                }
                operand.ingredients.push_back(access);
                operand.ingredients.push_back(index);

                if (look_ahead(TOKEN::DOT, lexemes)) {
                    operand.ingredients.push_back(
                        consume_token_of(TOKEN::DOT, lexemes));
                    operand.ingredients.push_back(
                        consume_token_of(TOKEN::LITERAL_ATOM, lexemes));
                }
            } else if (lexemes.front() == TOKEN::LITERAL_INTEGER) {
                auto const value =
                    consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
                operand.ingredients.push_back(value);
            } else if (lexemes.front() == TOKEN::LITERAL_FLOAT) {
                auto const value =
                    consume_token_of(TOKEN::LITERAL_FLOAT, lexemes);
                operand.ingredients.push_back(value);
            } else if (lexemes.front() == TOKEN::LITERAL_STRING) {
                auto const value =
                    consume_token_of(TOKEN::LITERAL_STRING, lexemes);
                operand.ingredients.push_back(value);
            } else if (lexemes.front() == TOKEN::LITERAL_ATOM) {
                auto const value =
                    consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
                operand.ingredients.push_back(value);
            } else {
                throw lexemes.front();
            }

            instruction.operands.push_back(std::move(operand));

            /*
             * Consume either a comma (meaning that there will be some more
             * operands), or a terminator (meaning that there will be no more
             * operands).
             */
            if (lexemes.front() == TOKEN::COMMA) {
                consume_token_of(TOKEN::COMMA, lexemes);
                continue;
            }
            if (lexemes.front() == TOKEN::TERMINATOR) {
                consume_token_of(TOKEN::TERMINATOR, lexemes);
                break;
            }
            throw lexemes.front();
        }

        fn->instructions.push_back(std::move(instruction));
        fn->instructions.back().physical_index = ins_physical_index++;
    }

    consume_token_of(TOKEN::END, lexemes);
    fn->end = consume_token_of(TOKEN::TERMINATOR, lexemes);

    fn->name = std::move(fn_name);

    return fn;
}

template<typename T> auto ston(std::string const& s) -> T
{
    if constexpr (std::is_signed_v<T>) {
        auto const full_width = std::stoll(s);
        auto const want_width = static_cast<T>(full_width);
        if (full_width != want_width) {
            throw std::out_of_range{"ston"};
        }
        return want_width;
    } else {
        auto const full_width = std::stoull(s);
        auto const want_width = static_cast<T>(full_width);
        if (full_width != want_width) {
            throw std::out_of_range{"ston"};
        }
        return want_width;
    }
}
auto parse_constant_definition(
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> std::unique_ptr<ast::Node>
{
    auto ct = std::make_unique<ast::Label_def>();

    using viua::libs::lexer::TOKEN;
    ct->start = consume_token_of(TOKEN::DEF_LABEL, lexemes);

    if (look_ahead(TOKEN::ATTR_LIST_OPEN, lexemes)) {
        ct->attributes = parse_attr_list(lexemes);
    }

    auto ct_name =
        consume_token_of({TOKEN::LITERAL_ATOM, TOKEN::LITERAL_STRING}, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);

    {
        /*
         * Constant definitions are spread on two lines: the first one gives the
         * name of the constant ie, the symbol by which it can be referenced;
         * and the second gives the value of the constant. It should look like
         * this:
         *
         *      .const: <name>
         *      .value: <type> <value-expression>
         *
         * Allowed value expressions depend on the type of the value, and each
         * is special cased to provide features useful for that kind of values.
         */
        consume_token_of(TOKEN::DEF_VALUE, lexemes);

        auto value_type =
            consume_token_of({TOKEN::LITERAL_ATOM, TOKEN::OPCODE}, lexemes);
        auto const known_types = std::set<std::string>{
            /*
             * Only strings allowed. Other types may be added later.
             */
            "string",
        };
        if (not known_types.contains(value_type.text)) {
            using viua::support::string::levenshtein_filter;
            auto misspell_candidates =
                levenshtein_filter(value_type.text, known_types);
            if (misspell_candidates.empty()) {
                throw value_type;
            }

            using viua::support::string::levenshtein_best;
            auto best_candidate =
                levenshtein_best(value_type.text,
                                 misspell_candidates,
                                 (value_type.text.size() / 2));
            if (best_candidate.second == value_type.text) {
                throw value_type;
            }

            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw did_you_mean(
                Error{value_type, Cause::Unknown_type, value_type.text},
                best_candidate.second);
        }

        if (value_type.text.front() == 'i' or value_type.text.front() == 'u') {
            ct->value.push_back(
                consume_token_of(TOKEN::LITERAL_INTEGER, lexemes));
        } else if (value_type == "float" or value_type == "double") {
            ct->value.push_back(
                consume_token_of(TOKEN::LITERAL_FLOAT, lexemes));
        } else if (value_type == "string") {
            do {
                ct->value.push_back(consume_token_of({TOKEN::LITERAL_ATOM,
                                                      TOKEN::LITERAL_STRING,
                                                      TOKEN::LITERAL_INTEGER,
                                                      TOKEN::RA_PTR_DEREF},
                                                     lexemes));
            } while (not look_ahead(TOKEN::TERMINATOR, lexemes));
        } else if (value_type == "atom") {
            ct->value.push_back(consume_token_of(TOKEN::LITERAL_ATOM, lexemes));
        }

        ct->type = std::move(value_type);
        consume_token_of(TOKEN::TERMINATOR, lexemes);
    }

    ct->name = std::move(ct_name);

    return ct;
}

auto parse(viua::support::vector_view<viua::libs::lexer::Lexeme> lexemes)
    -> std::vector<std::unique_ptr<ast::Node>>
{
    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};

    while (not lexemes.empty()) {
        auto const& each = lexemes.front();

        using viua::libs::lexer::TOKEN;
        if (each.token == TOKEN::DEF_FUNCTION) {
            auto node = parse_function_definition(lexemes);
            nodes.push_back(std::move(node));
        } else if (each.token == TOKEN::DEF_LABEL) {
            auto node = parse_constant_definition(lexemes);
            nodes.push_back(std::move(node));
        } else {
            throw each;
        }
    }

    return nodes;
}

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
               ast::Instruction const& each) -> void
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
        throw Error{raw_value, Cause::Invalid_operand, "expected integer"};
    }

    auto parts            = to_loading_parts_unsigned(value);
    auto const base       = parts.second.first.first;
    auto const multiplier = parts.second.first.second;
    auto const is_greedy  = (each.opcode.text.find("g.") == 0);

    auto const is_unsigned = (raw_value.text.back() == 'u');

    /*
     * Only use the luiu instruction of there's a reason to ie, if some
     * of the highest 36 bits are set. Otherwise, the lui is just
     * overhead.
     */
    if (parts.first) {
        using namespace std::string_literals;
        auto synth = each;
        synth.opcode.text =
            ((multiplier or base or is_greedy) ? "g.lui" : "lui");
        if (is_unsigned) {
            // FIXME loading signed values is ridiculously expensive and always
            // takes the most pessmisitic route - write a signed version of the
            // algorithm
            synth.opcode.text += 'u';
        }
        synth.operands.at(1).ingredients.front().text =
            std::to_string(parts.first);
        cooked.push_back(synth);
    }

    if (multiplier != 0) {
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.addiu";
            synth.physical_index = each.physical_index;

            auto const& lx = each.operands.front().ingredients.at(1);

            using viua::libs::lexer::TOKEN;
            {
                auto dst = ast::Operand{};
                dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                dst.ingredients.push_back(
                    lx.make_synth(std::to_string(std::stoull(lx.text) + 1),
                                  TOKEN::LITERAL_INTEGER));
                dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                dst.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));

                synth.operands.push_back(dst);
            }
            {
                auto src = ast::Operand{};
                src.ingredients.push_back(
                    lx.make_synth("void", TOKEN::RA_VOID));

                synth.operands.push_back(src);
            }
            {
                auto immediate = ast::Operand{};
                immediate.ingredients.push_back(lx.make_synth(
                    std::to_string(base), TOKEN::LITERAL_INTEGER));

                synth.operands.push_back(immediate);
            }

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.addiu";
            synth.physical_index = each.physical_index;

            auto const& lx = each.operands.front().ingredients.at(1);

            using viua::libs::lexer::TOKEN;
            {
                auto dst = ast::Operand{};
                dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                dst.ingredients.push_back(
                    lx.make_synth(std::to_string(std::stoull(lx.text) + 2),
                                  TOKEN::LITERAL_INTEGER));
                dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                dst.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));

                synth.operands.push_back(dst);
            }
            {
                auto src = ast::Operand{};
                src.ingredients.push_back(
                    lx.make_synth("void", TOKEN::RA_VOID));

                synth.operands.push_back(src);
            }
            {
                auto immediate = ast::Operand{};
                immediate.ingredients.push_back(lx.make_synth(
                    std::to_string(multiplier), TOKEN::LITERAL_INTEGER));

                synth.operands.push_back(immediate);
            }

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.mul";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 2);

            cooked.push_back(synth);
        }

        auto const remainder = parts.second.second;
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.addiu";
            synth.physical_index = each.physical_index;

            auto const& lx = each.operands.front().ingredients.at(1);

            using viua::libs::lexer::TOKEN;
            {
                auto dst = ast::Operand{};
                dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                dst.ingredients.push_back(
                    lx.make_synth(std::to_string(std::stoull(lx.text) + 2),
                                  TOKEN::LITERAL_INTEGER));
                dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                dst.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));

                synth.operands.push_back(dst);
            }
            {
                auto src = ast::Operand{};
                src.ingredients.push_back(
                    lx.make_synth("void", TOKEN::RA_VOID));

                synth.operands.push_back(src);
            }
            {
                auto immediate = ast::Operand{};
                immediate.ingredients.push_back(lx.make_synth(
                    std::to_string(remainder), TOKEN::LITERAL_INTEGER));

                synth.operands.push_back(immediate);
            }

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.add";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            synth.operands.push_back(synth.operands.back());

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 2);

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.add";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.push_back(each.operands.front());

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            cooked.push_back(synth);
        }

        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.delete";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            expand_delete(cooked, synth);
        }
        {
            using namespace std::string_literals;
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = (is_greedy ? "g." : "") + "delete"s;
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 2);

            expand_delete(cooked, synth);
        }
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
         *
         * Otherwise, we should increase the value stored by the lui
         * instruction.
         */
        if (parts.first == 0) {
            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.front().text = "void";
            synth.operands.back().ingredients.resize(1);
        } else {
            synth.operands.push_back(each.operands.front());
        }

        synth.operands.push_back(each.operands.back());
        synth.operands.back().ingredients.front().text = std::to_string(base);

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
auto expand_pseudoinstructions(std::vector<ast::Instruction> raw,
                               std::map<std::string, size_t> const& fn_offsets)
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
        } else if (each.opcode == "call") {
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

                auto const fn_name = each.operands.back().ingredients.front();
                if (fn_offsets.count(fn_name.text) == 0) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{fn_name, Cause::Call_to_undefined_function};
                }

                auto const fn_off = fn_offsets.at(fn_name.text);
                li.operands.back().ingredients.push_back(fn_name.make_synth(
                    std::to_string(fn_off) + 'u',
                    viua::libs::lexer::TOKEN::LITERAL_INTEGER));

                li.physical_index = each.physical_index;
            }
            expand_li(cooked, li);

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
        } else {
            /*
             * Real instructions should be pushed without any modification.
             */
            cooked.push_back(std::move(each));
        }

        logical_ops += (cooked.size() - logical_ops);

        /*
         * I think this part of the code might break if the value crosses a
         * threshold at which the alhorithm governing expansion of li chooses a
         * different instruction sequence. This code is a potential FIXME.
         *
         * Maybe we should do a second pass in which the actual costs are
         * calculated. After the whole sequence is analysed we know how many
         * physical instructions are there. Then, we know that eg, Nth logical
         * instruction will be converted into Mth physical instruction address.
         * If they have different costs we would have to adjust the mappings for
         * all instructions after that jump.
         *
         * But... this requires the target of the branch just analysed to have
         * to be recalculated because the physical instrution's layout just
         * changed. Damn, what a brain-dead design was chosen for the li
         * expansion. Costant size would be much better.
         * Another choice is to never REDUCE the cost, but insert a bunch of
         * g.noop's as padding. Yeah, that could work, I guess.
         */
        if (each.opcode == "if" or each.opcode == "g.if") {
            branch_ops_baggage +=
                li_cost(std::stoull(each.operands.back().to_string()));
        } else if (each.opcode == "jump" or each.opcode == "g.jump") {
            branch_ops_baggage +=
                li_cost(std::stoull(each.operands.back().to_string()));
        }
    }

    auto baked = std::vector<ast::Instruction>{};
    for (auto& each : cooked) {
        if (each.opcode == "if" or each.opcode == "g.if") {
            expand_if(baked, each, l2p);
        } else if (each.opcode == "jump" or each.opcode == "g.jump") {
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
            as_if.opcode.text    = (each.opcode == "jump") ? "if" : "g.if";
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
}  // namespace

namespace {
auto operand_or_throw(ast::Instruction const& insn, size_t const index)
    -> ast::Operand const&
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

auto emit_bytecode(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                   std::vector<viua::arch::instruction_type>& text,
                   std::vector<uint8_t>& fn_table,
                   std::map<std::string, size_t> const& fn_offsets)
    -> std::map<std::string, uint64_t>
{
    auto const ops_count =
        1
        + std::accumulate(
            nodes.begin(),
            nodes.end(),
            size_t{0},
            [](size_t const acc,
               std::unique_ptr<ast::Node> const& each) -> size_t {
                if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
                    return 0;
                }

                auto& fn = static_cast<ast::Fn_def&>(*each);
                return (acc + fn.instructions.size());
            });

    {
        text.reserve(ops_count);
        text.resize(ops_count);

        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.at(0) = N{static_cast<instruction_type>(OPCODE::HALT)}.encode();
    }

    auto fn_addresses = std::map<std::string, uint64_t>{};
    auto ip           = (text.data() + 1);
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        {
            /*
             * Save the function's address (offset into the .text section,
             * really) in the functions table. This is needed not only for
             * debugging, but also because the functions' addresses are resolved
             * dynamically for call and similar instructions. Why dynamically?
             *
             * Because there is a strong distinction between calls to bytecode
             * and foreign functions. At compile time, we don't yet know,
             * though, which function is foreign and which is bytecode.
             */
            auto const fn_addr =
                (ip - &text[0]) * sizeof(viua::arch::instruction_type);
            fn_addresses[fn.name.text] = fn_addr;
            patch_fn_address(fn_table, fn_offsets.at(fn.name.text), fn_addr);
        }

        for (auto const& insn : fn.instructions) {
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
                *ip++ = static_cast<uint64_t>(opcode);
                break;
            case FORMAT::T:
                *ip++ =
                    viua::arch::ops::T{opcode,
                                       operand_or_throw(insn, 0).make_access(),
                                       operand_or_throw(insn, 1).make_access(),
                                       operand_or_throw(insn, 2).make_access()}
                        .encode();
                break;
            case FORMAT::D:
                *ip++ =
                    viua::arch::ops::D{opcode,
                                       operand_or_throw(insn, 0).make_access(),
                                       operand_or_throw(insn, 1).make_access()}
                        .encode();
                break;
            case FORMAT::S:
                *ip++ =
                    viua::arch::ops::S{opcode,
                                       operand_or_throw(insn, 0).make_access()}
                        .encode();
                break;
            case FORMAT::F:
                break;  // FIXME
            case FORMAT::E:
                *ip++ =
                    viua::arch::ops::E{
                        opcode,
                        operand_or_throw(insn, 0).make_access(),
                        std::stoull(
                            operand_or_throw(insn, 1).ingredients.front().text)}
                        .encode();
                break;
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
                    *ip++ =
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
                break;
            }
            }
        }
    }

    return fn_addresses;
}
}  // namespace

namespace {
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

auto display_error_and_exit [[noreturn]] (std::string_view source_path,
                                          std::string_view source_text,
                                          viua::libs::lexer::Lexeme const& e)
-> void
{
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

    {
        auto const location = e.location;

        auto line = view_line_of(source_text, location);

        source_line << esc(2, COLOR_FG_RED) << std::setw(LINE_NO_WIDTH)
                    << (location.line + 1) << esc(2, ATTR_RESET) << SEPARATOR;
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

    std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
              << ':' << esc(2, COLOR_FG_WHITE) << (e.location.line + 1)
              << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
              << (e.location.character + 1) << esc(2, ATTR_RESET) << ": "
              << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET) << ": "
              << "unexpected token: " << viua::libs::lexer::to_string(e.token)
              << "\n";

    exit(1);
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
    [[noreturn]] (std::string_view source_path,
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

    std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
              << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1)
              << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
              << (e.character() + 1) << esc(2, ATTR_RESET) << ": "
              << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET) << ": "
              << e.str() << "\n";

    for (auto const& each : e.notes()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
                  << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1)
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (e.character() + 1) << esc(2, ATTR_RESET) << ": "
                  << esc(2, COLOR_FG_CYAN) << "note" << esc(2, ATTR_RESET)
                  << ": ";
        if (each.find('\n') == std::string::npos) {
            std::cerr << each << "\n";
        } else {
            auto const prefix_length =
                source_path.size() + std::to_string(e.line() + 1).size()
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

auto display_error_in_function(std::string_view source_path,
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

    std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
              << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1)
              << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
              << (e.character() + 1) << esc(2, ATTR_RESET) << ": "
              << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
              << ": in function " << esc(2, COLOR_FG_WHITE) << fn_name
              << esc(2, ATTR_RESET) << ":\n";
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    if (argc == 1) {
        return 1;
    }

    auto args = std::vector<std::string_view>{};
    std::copy(argv + 1, argv + argc, std::back_inserter(args));

    auto output_path = std::string{"a.out"};
    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "-o") {
            output_path = args.at(++i);
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
    auto const source_path = args.back();
    auto source_text       = std::string{};
    {
        auto const source_fd = open(source_path.data(), O_RDONLY);
        if (source_fd == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = strerrordesc_np(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }

        struct stat source_stat {
        };
        if (fstat(source_fd, &source_stat) == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = strerrordesc_np(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path
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

            std::cerr << esc(2, COLOR_FG_WHITE) << source_path
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET)
                      << ": empty source file\n";
            return 1;
        }

        source_text.resize(source_stat.st_size);
        read(source_fd, source_text.data(), source_text.size());
        close(source_fd);
    }

    /*
     * Lexical analysis (lexing).
     *
     * Split the loaded source code into a stream of lexemes for easier
     * processing later. The first point at which errors are detected eg, if
     * illegal characters are used, strings are unclosed, etc.
     */
    auto lexemes = std::vector<viua::libs::lexer::Lexeme>{};
    try {
        lexemes = viua::libs::lexer::lex(source_text);
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
            auto line = view_line_of(source_text, location);

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

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
                  << ':' << esc(2, COLOR_FG_WHITE) << (location.line + 1)
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (location.character + 1) << esc(2, ATTR_RESET) << ": "
                  << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": "
                  << "no token match at character "
                  << viua::support::string::CORNER_QUOTE_LL
                  << esc(2, COLOR_FG_WHITE) << e.text << esc(2, ATTR_RESET)
                  << viua::support::string::CORNER_QUOTE_UR << "\n";

        return 1;
    } catch (viua::libs::errors::compile_time::Error const& e) {
        display_error_and_exit(source_path, source_text, e);
    }

    if constexpr (DEBUG_LEX) {
        std::cerr << lexemes.size() << " raw lexeme(s)\n";
        for (auto const& each : lexemes) {
            std::cerr << "  " << viua::libs::lexer::to_string(each.token) << ' '
                      << each.location.line << ':' << each.location.character
                      << '-' << (each.location.character + each.text.size() - 1)
                      << " +" << each.location.offset;

            using viua::libs::lexer::TOKEN;
            auto const printable = (each.token == TOKEN::LITERAL_STRING)
                                   or (each.token == TOKEN::LITERAL_INTEGER)
                                   or (each.token == TOKEN::LITERAL_FLOAT)
                                   or (each.token == TOKEN::LITERAL_ATOM)
                                   or (each.token == TOKEN::OPCODE);
            if (printable) {
                std::cerr << " " << each.text;
            }

            std::cerr << "\n";
        }
    }

    lexemes = ast::remove_noise(std::move(lexemes));

    if constexpr (DEBUG_LEX) {
        std::cerr << lexemes.size() << " cooked lexeme(s)\n";
        if constexpr (false) {
            for (auto const& each : lexemes) {
                std::cerr << "  " << viua::libs::lexer::to_string(each.token)
                          << ' ' << each.location.line << ':'
                          << each.location.character << '-'
                          << (each.location.character + each.text.size() - 1)
                          << " +" << each.location.offset;

                using viua::libs::lexer::TOKEN;
                auto const printable = (each.token == TOKEN::LITERAL_STRING)
                                       or (each.token == TOKEN::LITERAL_INTEGER)
                                       or (each.token == TOKEN::LITERAL_FLOAT)
                                       or (each.token == TOKEN::LITERAL_ATOM)
                                       or (each.token == TOKEN::OPCODE);
                if (printable) {
                    std::cerr << " " << each.text;
                }

                std::cerr << "\n";
            }
        }
    }

    /*
     * Syntactical analysis (parsing).
     *
     * Convert raw stream of lexemes into an abstract syntax tree structure that
     * groups lexemes representing a single entity (eg, a register access
     * specification) into a single object, and represents the relationships
     * between such objects.
     */
    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};
    try {
        nodes = parse(lexemes);
    } catch (viua::libs::lexer::Lexeme const& e) {
        display_error_and_exit(source_path, source_text, e);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        display_error_and_exit(source_path, source_text, e);
    }

    /*
     * Calculate function spans in source code for error reporting. This way an
     * error offset can be matched to a function without the error having to
     * carry the function name.
     */
    auto fn_spans =
        std::vector<std::pair<std::string, std::pair<size_t, size_t>>>{};
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        fn_spans.emplace_back(
            fn.name.text,
            std::pair<size_t, size_t>{fn.start.location.offset,
                                      fn.end.location.offset});
    }

    /*
     * String table preparation.
     *
     * Replace string, atom, float, and double literals in operands with offsets
     * into the string table. We want all instructions to fit into 64 bits, so
     * having variable-size operands is not an option.
     *
     * Don't move the strings table preparation after the pseudoinstruction
     * expansion stage. li pseudoinstructions are emitted during strings table
     * preparation so they need to be expanded.
     */
    auto strings_table = std::vector<uint8_t>{};
    auto var_offsets   = std::map<std::string, size_t>{};
    auto fn_table      = std::vector<uint8_t>{};
    auto fn_offsets    = std::map<std::string, size_t>{};
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Label_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& ct = static_cast<ast::Label_def&>(*each);
        if (ct.type == "string") {
            auto s = std::string{};
            for (auto i = size_t{0}; i < ct.value.size(); ++i) {
                auto& each = ct.value.at(i);

                using enum viua::libs::lexer::TOKEN;
                if (each.token == LITERAL_STRING) {
                    auto tmp = each.text;
                    tmp      = tmp.substr(1, tmp.size() - 2);
                    tmp      = viua::support::string::unescape(tmp);
                    s += tmp;
                } else if (each.token == RA_PTR_DEREF) {
                    auto& next = ct.value.at(++i);
                    if (next.token != LITERAL_INTEGER) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto const e = Error{each,
                                             Cause::Invalid_operand,
                                             "cannot multiply string constant "
                                             "by non-integer"}
                                           .add(next)
                                           .add(ct.value.at(i - 2))
                                           .aside("right-hand side must be an "
                                                  "positive integer");
                        display_error_and_exit(source_path, source_text, e);
                    }

                    auto x = ston<size_t>(next.text);
                    auto o = std::ostringstream{};
                    for (auto i = size_t{0}; i < x; ++i) {
                        o << s;
                    }
                    s = o.str();
                }
            }

            var_offsets[ct.name.text] = save_string(strings_table, s);
        }
    }
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        fn_offsets.emplace(fn.name.text,
                           save_fn_address(fn_table, fn.name.text));

        auto cooked = std::vector<ast::Instruction>{};
        for (auto& insn : fn.instructions) {
            if (insn.opcode == "atom" or insn.opcode == "g.atom") {
                auto const lx = insn.operands.back().ingredients.front();
                auto s        = lx.text;
                if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
                    s = s.substr(1, s.size() - 2);
                    s = viua::support::string::unescape(s);
                } else if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                    // do nothing
                } else {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{
                        lx, Cause::Invalid_operand, "expected atom or string"};
                }
                auto const saved_at = save_string(strings_table, s);

                auto synth           = ast::Instruction{};
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
                } else if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                    try {
                        saved_at = var_offsets.at(lx.text);
                    } catch (std::out_of_range const&) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto e = Error{lx, Cause::Unknown_label, lx.text};

                        using viua::support::string::levenshtein_filter;
                        auto misspell_candidates =
                            levenshtein_filter(lx.text, var_offsets);
                        if (not misspell_candidates.empty()) {
                            using viua::support::string::levenshtein_best;
                            auto best_candidate =
                                levenshtein_best(lx.text,
                                                 misspell_candidates,
                                                 (lx.text.size() / 2));
                            if (best_candidate.second != lx.text) {
                                did_you_mean(e, best_candidate.second);
                            }
                        }

                        display_error_in_function(source_path, e, fn.name.text);
                        display_error_and_exit(source_path, source_text, e);
                    }
                } else {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;

                    auto const e = Error{lx, Cause::Invalid_operand}.aside(
                        "expected string literal, or a label");
                    display_error_in_function(source_path, e, fn.name.text);
                    display_error_and_exit(source_path, source_text, e);
                }

                auto synth           = ast::Instruction{};
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

                auto synth           = ast::Instruction{};
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

                auto synth           = ast::Instruction{};
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
        }
        fn.instructions = std::move(cooked);
    }

    /*
     * Pseudoinstruction- and macro-expansion.
     *
     * Replace pseudoinstructions (eg, li) with sequences of real instructions
     * that will have the same effect. Ditto for macros.
     */
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn                 = static_cast<ast::Fn_def&>(*each);
        auto const raw_ops_count = fn.instructions.size();
        try {
            fn.instructions = expand_pseudoinstructions(
                std::move(fn.instructions), fn_offsets);
        } catch (viua::libs::errors::compile_time::Error const& e) {
            display_error_in_function(source_path, e, fn.name.text);
            display_error_and_exit(source_path, source_text, e);
        }

        if constexpr (DEBUG_EXPANSION) {
            std::cerr << "FN " << fn.to_string() << " with " << raw_ops_count
                      << " raw, " << fn.instructions.size() << " baked op(s)\n";
            auto physical_index = size_t{0};
            for (auto const& op : fn.instructions) {
                std::cerr << "  " << std::setw(4) << std::setfill('0')
                          << std::hex << physical_index++ << " " << std::setw(4)
                          << std::setfill('0') << std::hex << op.physical_index
                          << "  " << op.to_string() << "\n";
            }
        }
    }

    /*
     * Detect entry point function.
     *
     * We're not handling relocatable files (shared libs, etc) yet so it makes
     * sense to enforce entry function presence in all cases. Once the
     * relocatables and separate compilation is supported again, this should be
     * hidden behind a flag.
     */
    auto entry_point_fn = std::optional<viua::libs::lexer::Lexeme>{};
    for (auto const& each : nodes) {
        if (each->has_attr("entry_point")) {
            if (entry_point_fn.has_value()) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto const dup = static_cast<ast::Fn_def&>(*each);
                auto const e =
                    Error{
                        dup.name, Cause::Duplicated_entry_point, dup.name.text}
                        .add(dup.attr("entry_point").value())
                        .note("first entry point was: " + entry_point_fn->text);
                display_error_and_exit(source_path, source_text, e);
            }
            entry_point_fn = static_cast<ast::Fn_def&>(*each).name;
        }
    }
    if (not entry_point_fn.has_value()) {
        using viua::support::tty::ATTR_RESET;
        using viua::support::tty::COLOR_FG_CYAN;
        using viua::support::tty::COLOR_FG_RED;
        using viua::support::tty::COLOR_FG_WHITE;
        using viua::support::tty::send_escape_seq;
        constexpr auto esc = send_escape_seq;

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
                  << ": " << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": "
                  << " no entry point function defined\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
                  << ": " << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": "
                  << " the entry function should have the [[entry_point]] "
                     "attribute\n";
        return 1;
    }

    /*
     * Bytecode emission.
     *
     * This stage is also responsible for preparing the function table. It is a
     * table mapping function names to the offsets inside the .text section, at
     * which their entry points reside.
     */
    auto text         = std::vector<viua::arch::instruction_type>{};
    auto fn_addresses = std::map<std::string, uint64_t>{};
    try {
        fn_addresses = emit_bytecode(nodes, text, fn_table, fn_offsets);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        auto fn_name = std::optional<std::string>{};

        for (auto const& [name, offs] : fn_spans) {
            auto const [low, high] = offs;
            auto const off         = e.location().offset;
            if ((off >= low) and (off <= high)) {
                fn_name = name;
            }
        }

        if (fn_name.has_value()) {
            display_error_in_function(source_path, e, *fn_name);
        }
        display_error_and_exit(source_path, source_text, e);
    }

    /*
     * ELF emission.
     */
    {
        auto const a_out =
            open(output_path.c_str(),
                 O_CREAT | O_TRUNC | O_WRONLY,
                 S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        constexpr auto VIUA_MAGIC [[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
        auto const VIUAVM_INTERP                   = std::string{"viua-vm"};

        {
            constexpr auto NO_OF_ELF_PHDR_USED = 5;

            auto const text_size =
                (text.size() * sizeof(decltype(text)::value_type));
            auto const text_offset =
                (sizeof(Elf64_Ehdr) + (NO_OF_ELF_PHDR_USED * sizeof(Elf64_Phdr))
                 + (VIUAVM_INTERP.size() + 1));
            auto const strings_offset = (text_offset + text_size);
            auto const fn_offset      = (strings_offset + strings_table.size());

            if constexpr (DEBUG_ELF) {
                using viua::support::tty::ATTR_RESET;
                using viua::support::tty::COLOR_FG_WHITE;
                using viua::support::tty::send_escape_seq;
                constexpr auto esc = send_escape_seq;

                auto const location = entry_point_fn.value().location;

                std::cerr << esc(2, COLOR_FG_WHITE) << source_path
                          << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                          << (location.line + 1) << esc(2, ATTR_RESET) << ':'
                          << esc(2, COLOR_FG_WHITE) << (location.character + 1)
                          << esc(2, ATTR_RESET)
                          << ": text segment size: " << esc(2, COLOR_FG_WHITE)
                          << text_size << esc(2, ATTR_RESET) << " byte(s)"
                          << "\n";
            }

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
            elf_header.e_type                 = ET_EXEC;
            elf_header.e_machine              = ET_NONE;
            elf_header.e_version              = elf_header.e_ident[EI_VERSION];
            elf_header.e_entry =
                text_offset + fn_addresses[entry_point_fn.value().text];
            elf_header.e_phoff     = sizeof(elf_header);
            elf_header.e_phentsize = sizeof(Elf64_Phdr);
            elf_header.e_phnum     = NO_OF_ELF_PHDR_USED;
            elf_header.e_shoff     = 0;  // FIXME section header table
            elf_header.e_flags  = 0;  // processor-specific flags, should be 0
            elf_header.e_ehsize = sizeof(elf_header);
            write(a_out, &elf_header, sizeof(elf_header));

            Elf64_Phdr magic_for_binfmt_misc{};
            magic_for_binfmt_misc.p_type   = PT_NULL;
            magic_for_binfmt_misc.p_offset = 0;
            memcpy(&magic_for_binfmt_misc.p_offset, VIUA_MAGIC, 8);
            write(a_out, &magic_for_binfmt_misc, sizeof(magic_for_binfmt_misc));

            Elf64_Phdr interpreter{};
            interpreter.p_type = PT_INTERP;
            interpreter.p_offset =
                (sizeof(elf_header) + NO_OF_ELF_PHDR_USED * sizeof(Elf64_Phdr));
            interpreter.p_filesz = VIUAVM_INTERP.size() + 1;
            interpreter.p_flags  = PF_R;
            write(a_out, &interpreter, sizeof(interpreter));

            Elf64_Phdr text_segment{};
            text_segment.p_type   = PT_LOAD;
            text_segment.p_offset = text_offset;
            text_segment.p_filesz = text_size;
            text_segment.p_memsz  = text_size;
            text_segment.p_flags  = PF_R | PF_X;
            text_segment.p_align  = sizeof(viua::arch::instruction_type);
            write(a_out, &text_segment, sizeof(text_segment));

            Elf64_Phdr strings_segment{};
            strings_segment.p_type   = PT_LOAD;
            strings_segment.p_offset = strings_offset;
            strings_segment.p_filesz = strings_table.size();
            strings_segment.p_memsz  = strings_table.size();
            strings_segment.p_flags  = PF_R;
            strings_segment.p_align  = sizeof(viua::arch::instruction_type);
            write(a_out, &strings_segment, sizeof(strings_segment));

            Elf64_Phdr fn_segment{};
            fn_segment.p_type   = PT_LOAD;
            fn_segment.p_offset = fn_offset;
            fn_segment.p_filesz = fn_table.size();
            fn_segment.p_memsz  = fn_table.size();
            fn_segment.p_flags  = PF_R;
            fn_segment.p_align  = sizeof(viua::arch::instruction_type);
            write(a_out, &fn_segment, sizeof(fn_segment));

            write(a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

            write(a_out, text.data(), text_size);

            write(a_out, strings_table.data(), strings_table.size());

            write(a_out, fn_table.data(), fn_table.size());
        }

        close(a_out);
    }

    return 0;
}
