/*
 *  Copyright (C) 2022-2023 Marek Marecki
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

#include <viua/libs/parser.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/core.h>


namespace viua::libs::parser {
namespace ast {
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
    if (lx != TOKEN::DOLLAR) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{ingredients.front(), Cause::Invalid_register_access};
    }

    auto const direct = (lx == TOKEN::DOLLAR);
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

auto Fn_def::to_string() const -> std::string
{
    return viua::libs::lexer::to_string(viua::libs::lexer::TOKEN::DEF_FUNCTION)
           + ' ' + std::to_string(name.location.line + 1) + ':'
           + std::to_string(name.location.character + 1) + '-'
           + std::to_string(name.location.character + name.text.size()) + ' '
           + name.text;
}

auto Label_def::to_string() const -> std::string
{
    return viua::libs::lexer::to_string(viua::libs::lexer::TOKEN::DEF_LABEL)
           + ' ' + std::to_string(name.location.line + 1) + ':'
           + std::to_string(name.location.character + 1) + '-'
           + std::to_string(name.location.character + name.text.size()) + ' '
           + name.text;
}
}  // namespace ast

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

auto consume_token_of(
    viua::libs::lexer::TOKEN const tt,
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> viua::libs::lexer::Lexeme
{
    if (lexemes.front().token != tt) {
        throw lexemes.front();
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

auto parse_instruction(
    viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> ast::Instruction
{
    auto instruction = ast::Instruction{};

    using viua::libs::lexer::TOKEN;
    try {
        instruction.opcode = consume_token_of(TOKEN::OPCODE, lexemes);
    } catch (viua::libs::lexer::Lexeme const& e) {
        if (e.token != viua::libs::lexer::TOKEN::LITERAL_ATOM) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unexpected_token, e.text};
        }

        using viua::libs::lexer::OPCODE_NAMES;
        using viua::support::string::levenshtein_filter;
        auto misspell_candidates = levenshtein_filter(e.text, OPCODE_NAMES);
        if (misspell_candidates.empty()) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unexpected_token, e.text};
        }

        using viua::support::string::levenshtein_best;
        auto best_candidate =
            levenshtein_best(e.text, misspell_candidates, (e.text.size() / 2));
        if (best_candidate.second == e.text) {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{e, Cause::Unexpected_token, e.text};
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

    auto const fundamental_type_names = std::map<std::string, uint8_t>{
        {"int", static_cast<uint8_t>(viua::vm::Register::Types::INT)},
        {"uint", static_cast<uint8_t>(viua::vm::Register::Types::UINT)},
        {"float", static_cast<uint8_t>(viua::vm::Register::Types::FLOAT32)},
        {"double", static_cast<uint8_t>(viua::vm::Register::Types::FLOAT64)},
        {"pointer", static_cast<uint8_t>(viua::vm::Register::Types::POINTER)},
        {"atom", static_cast<uint8_t>(viua::vm::Register::Types::ATOM)},
        {"pid", static_cast<uint8_t>(viua::vm::Register::Types::PID)},
    };

    auto valid_cast =
        [&lexemes, &instruction, &fundamental_type_names]() -> bool {
        if (instruction.opcode != "cast" and instruction.opcode != "g.cast") {
            return false;
        }

        /*
         * The type specifier (which in some cases looks like an instruction
         * name) is only valid in the second position.
         */
        if (instruction.operands.size() != 1) {
            return false;
        }

        return fundamental_type_names.count(lexemes.front().text);
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
                    .aside("register index range is 0-"
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
        } else if (look_ahead(TOKEN::AT, lexemes)) {
            auto const access = consume_token_of(TOKEN::AT, lexemes);
            auto atom         = viua::libs::lexer::Lexeme{};
            try {
                atom = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
            } catch (viua::libs::lexer::Lexeme const& e) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{e, Cause::Unexpected_token}.add(access).aside(
                    "label name must be an atom");
            }
            operand.ingredients.push_back(access);
            operand.ingredients.push_back(atom);
        } else if (valid_cast()) {
            auto const value =
                consume_token_of({TOKEN::OPCODE, TOKEN::LITERAL_ATOM}, lexemes);
            operand.ingredients.push_back(value);
            auto& tt = operand.ingredients.front().text;
            tt       = std::to_string(fundamental_type_names.at(tt));
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
        } else {
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{
                lexemes.front(), Cause::Unexpected_token, "cannot parse"};
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

        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{lexemes.front(), Cause::Unexpected_token}.note(
            "expected a comma, or a newline");
    }

    return instruction;
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
    fn->name = std::move(fn_name);

    try {
        consume_token_of(TOKEN::TERMINATOR, lexemes);
    } catch (viua::libs::lexer::Lexeme const& e) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{lexemes.front(),
                    Cause::Unexpected_token,
                    "in definition of a function"}
            .add(fn->start);
    }

    if (fn->name.token == TOKEN::LITERAL_STRING) {
        fn->name.token = TOKEN::LITERAL_ATOM;
        fn->name.text =
            fn->name.text.substr(1, fn->name.text.substr().size() - 2);
    }

    /*
     * This function is not defined in the current module, therefore we should
     * not attempt to parse its instructions.
     */
    if (fn->has_attr("extern")) {
        return fn;
    }

    auto instructions       = std::vector<std::unique_ptr<ast::Node>>{};
    auto ins_physical_index = size_t{0};
    while ((not lexemes.empty()) and lexemes.front() != TOKEN::END) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        try {
            auto instruction = parse_instruction(lexemes);
            fn->instructions.push_back(std::move(instruction));
            fn->instructions.back().physical_index = ins_physical_index++;
        } catch (Error& e) {
            throw Error{fn->name,
                        Cause::None,
                        "in definition of function " + fn->name.text}
                .add(fn->start)
                .chain(std::move(e));
        }
    }

    consume_token_of(TOKEN::END, lexemes);
    fn->end = consume_token_of(TOKEN::TERMINATOR, lexemes);

    return fn;
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
    ct->name = std::move(ct_name);
    consume_token_of(TOKEN::TERMINATOR, lexemes);

    /*
     * This object is not defined in the current module, therefore we should
     * not attempt to parse its value.
     */
    if (ct->has_attr("extern")) {
        return ct;
    }

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
            "atom",
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
            using viua::libs::errors::compile_time::Cause;
            using viua::libs::errors::compile_time::Error;
            throw Error{each, Cause::Unexpected_token}
                .aside("only directives can be top-level tokens")
                .note("refer to viua-asm-lang(1) for more information");
        }
    }

    return nodes;
}
}  // namespace viua::libs::parser
