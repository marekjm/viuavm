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

#ifndef VIUA_LIBS_PARSER_H
#define VIUA_LIBS_PARSER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>
#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/support/string.h>
#include <viua/support/vector.h>


namespace viua::libs::parser {
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

struct Operand : Node {
    std::vector<viua::libs::lexer::Lexeme> ingredients;

    auto to_string() const -> std::string override;

    auto make_access() const -> viua::arch::Register_access;

    virtual ~Operand() = default;
};

struct Instruction : Node {
    viua::libs::lexer::Lexeme opcode;
    std::vector<Operand> operands;

    size_t physical_index{static_cast<size_t>(-1)};

    auto to_string() const -> std::string override;
    auto parse_opcode() const -> viua::arch::opcode_type;

    virtual ~Instruction() = default;
};

struct Fn_def : Node {
    viua::libs::lexer::Lexeme name;
    std::vector<Instruction> instructions;

    viua::libs::lexer::Lexeme start;
    viua::libs::lexer::Lexeme end;

    auto to_string() const -> std::string override;

    virtual ~Fn_def() = default;
};

struct Label_def : Node {
    viua::libs::lexer::Lexeme name;
    viua::libs::lexer::Lexeme type;
    std::vector<viua::libs::lexer::Lexeme> value;

    viua::libs::lexer::Lexeme start;
    viua::libs::lexer::Lexeme end;

    auto to_string() const -> std::string override;

    virtual ~Label_def() = default;
};

auto remove_noise(std::vector<viua::libs::lexer::Lexeme>)
    -> std::vector<viua::libs::lexer::Lexeme>;
}  // namespace ast

auto did_you_mean(viua::libs::errors::compile_time::Error&&, std::string)
    -> viua::libs::errors::compile_time::Error;
auto did_you_mean(viua::libs::errors::compile_time::Error&, std::string)
    -> viua::libs::errors::compile_time::Error&;

auto parse_instruction(viua::support::vector_view<viua::libs::lexer::Lexeme>&)
    -> ast::Instruction;
auto parse(viua::support::vector_view<viua::libs::lexer::Lexeme>)
    -> std::vector<std::unique_ptr<ast::Node>>;
}  // namespace viua::libs::parser

#endif
