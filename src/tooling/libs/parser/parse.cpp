/*
 *  Copyright (C) 2018-2019 Marek Marecki
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

#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <viua/bytecode/maps.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/lexer/classifier.h>
#include <viua/tooling/libs/lexer/tokenise.h>
#include <viua/tooling/libs/parser/parser.h>
#include <viua/util/string/ops.h>
#include <viua/util/vector_view.h>

namespace viua { namespace tooling { namespace libs { namespace parser {
using viua::util::vector_view;
using index_type = std::vector<viua::tooling::libs::lexer::Token>::size_type;

auto Fragment::type() const -> Fragment_type {
    return fragment_type;
}

auto Fragment::add(viua::tooling::libs::lexer::Token tok) -> void {
    fragment_tokens.push_back(std::move(tok));
}

auto Fragment::tokens() const -> decltype(fragment_tokens) const& {
    return fragment_tokens;
}

auto Fragment::token(Tokens_size_type const n) const
    -> viua::tooling::libs::lexer::Token const& {
    return fragment_tokens.at(n);
}

Fragment::Fragment(Fragment_type t) : fragment_type{t} {}

Extern_function::Extern_function(std::string fn, uint64_t const a)
        : Fragment{Fragment_type::Extern_function}
        , function_name{std::move(fn)}
        , arity{a} {}

Extern_block::Extern_block(std::string bn)
        : Fragment{Fragment_type::Extern_block}, block_name{std::move(bn)} {}

End_directive::End_directive() : Fragment{Fragment_type::End_directive} {}

Function_head::Function_head(std::string fn,
                             uint64_t const a,
                             std::set<std::string> attrs)
        : Fragment{Fragment_type::Function_head}
        , function_name{std::move(fn)}
        , arity{a}
        , attributes{std::move(attrs)} {}

Closure_head::Closure_head(std::string fn,
                           uint64_t const a,
                           std::set<std::string> attrs)
        : Fragment{Fragment_type::Closure_head}
        , function_name{std::move(fn)}
        , arity{a}
        , attributes{std::move(attrs)} {}

Block_head::Block_head(std::string bn, std::set<std::string> attrs)
        : Fragment{Fragment_type::Block_head}
        , name{std::move(bn)}
        , attributes{std::move(attrs)} {}

Import_directive::Import_directive(std::string bn, std::set<std::string> attrs)
        : Fragment{Fragment_type::Import_directive}
        , module_name{std::move(bn)}
        , attributes{std::move(attrs)} {}

Info_directive::Info_directive(std::string k, std::string v)
        : Fragment{Fragment_type::Info_directive}
        , key{std::move(k)}
        , value{std::move(v)} {}

Mark_directive::Mark_directive(std::string m)
        : Fragment{Fragment_type::Mark_directive}, mark{std::move(m)} {}

Name_directive::Name_directive(viua::internals::types::register_index const ri,
                               bool const i,
                               std::string n)
        : Fragment{Fragment_type::Name_directive}
        , register_index{ri}
        , iota{i}
        , name{std::move(n)} {}

auto Operand::type() const -> Operand_type {
    return operand_type;
}

auto Operand::add(viua::tooling::libs::lexer::Token t) -> void {
    operand_tokens.push_back(t);
}

auto Operand::tokens() const -> decltype(operand_tokens) const& {
    return operand_tokens;
}

Operand::Operand(Operand_type const o) : operand_type{o} {}

Register_address::Register_address(
    viua::internals::types::register_index const ri,
    bool const i,
    bool const n,
    viua::internals::Register_sets const rs,
    viua::internals::Access_specifier const as)
        : Operand{Operand_type::Register_address}
        , index{ri}
        , iota{i}
        , name{n}
        , register_set{rs}
        , access{as} {}

Integer_literal::Integer_literal(viua::types::Integer::underlying_type const x)
        : Operand{Operand_type::Integer_literal}, n{x} {}

Float_literal::Float_literal(viua::types::Float::underlying_type const x)
        : Operand{Operand_type::Float_literal}, n{x} {}

Text_literal::Text_literal(std::string s)
        : Operand{Operand_type::Text_literal}, text{std::move(s)} {}

Atom_literal::Atom_literal(std::string s)
        : Operand{Operand_type::Atom_literal}, text{std::move(s)} {}

Bits_literal::Bits_literal(std::string s)
        : Operand{Operand_type::Bits_literal}, text{std::move(s)} {}

Boolean_literal::Boolean_literal(bool const v)
        : Operand{Operand_type::Boolean_literal}, value{v} {}

Void::Void() : Operand{Operand_type::Void} {}

Function_name::Function_name(std::string fn, uint64_t const a)
        : Operand{Operand_type::Function_name}
        , function_name{std::move(fn)}
        , arity{a} {}

Block_name::Block_name(std::string bn)
        : Operand{Operand_type::Block_name}, name{std::move(bn)} {}

Module_name::Module_name(std::string bn)
        : Operand{Operand_type::Module_name}, name{std::move(bn)} {}

Timeout_literal::Timeout_literal(std::string s)
        : Operand{Operand_type::Timeout_literal}, value{std::move(s)} {}

Jump_offset::Jump_offset(std::string s)
        : Operand{Operand_type::Jump_offset}, value{std::move(s)} {}

Jump_label::Jump_label(std::string s)
        : Operand{Operand_type::Jump_label}, value{std::move(s)} {}

Instruction::Instruction(OPCODE const o)
        : Fragment{Fragment_type::Instruction}, opcode{o} {}

static auto parse_extern_function_directive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto frag = std::make_unique<Extern_function>(
        tokens.at(1).str(), std::stoull(tokens.at(3).str()));

    frag->add(tokens.at(0));  // .extern_function:
    frag->add(tokens.at(1));  // name
    frag->add(tokens.at(2));  // arity separator
    frag->add(tokens.at(3));  // arity

    fragments.push_back(std::move(frag));

    return 4;
}

static auto parse_extern_block_directive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto frag = std::make_unique<Extern_block>(tokens.at(1).str());

    frag->add(tokens.at(0));  // .extern_block:
    frag->add(tokens.at(1));  // name

    fragments.push_back(std::move(frag));

    return 2;
}

template<typename T>
auto parse_function_head(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    ++i;  // .function: or .closure:

    auto attributes = std::set<std::string>{};
    ++i;  // opening of the attribute list "[["

    if (tokens.at(i) != "]]") {
        // first attribute
        attributes.insert(tokens.at(i++).str());

        // all the rest
        while (tokens.at(i) != "]]") {
            // the "," between attributes
            ++i;

            // the attribute
            attributes.insert(tokens.at(i++).str());
        }
    }

    ++i;  // closing of the attribute list "]]"

    auto frag =
        std::make_unique<T>(tokens.at(i).str()  // name
                            ,
                            std::stoull(tokens.at(i + 2).str())  // arity
                            ,
                            std::move(attributes));
    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }
    frag->add(tokens.at(i++));  // name
    frag->add(tokens.at(i++));  // arity separator (/)
    frag->add(tokens.at(i++));  // arity

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_block_head(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    ++i;  // .block:

    auto attributes = std::set<std::string>{};
    ++i;  // opening of the attribute list "[["

    if (tokens.at(i) != "]]") {
        // first attribute
        attributes.insert(tokens.at(i++).str());

        // all the rest
        while (tokens.at(i) != "]]") {
            // the "," between attributes
            ++i;

            // the attribute
            attributes.insert(tokens.at(i++).str());
        }
    }

    ++i;  // closing of the attribute list "]]"

    auto frag = std::make_unique<Block_head>(tokens.at(i).str()  // name
                                             ,
                                             std::move(attributes));
    frag->add(tokens.at(0));
    frag->add(tokens.at(i++));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_name_directive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto const is_iota = (tokens.at(1) == "iota");
    auto const index   = static_cast<viua::internals::types::register_index>(
        is_iota ? 0 : std::stoul(tokens.at(1).str()));

    auto frag =
        std::make_unique<Name_directive>(index, is_iota, tokens.at(2).str());
    frag->add(tokens.at(i++));  // .name:
    frag->add(tokens.at(i++));  // index
    frag->add(tokens.at(i++));  // token

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_info_directive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Info_directive>(tokens.at(1).str(),
                                                 tokens.at(2).str());
    frag->add(tokens.at(i++));  // .info:
    frag->add(tokens.at(i++));  // <key>
    frag->add(tokens.at(i++));  // <value>

    fragments.push_back(std::move(frag));

    return i;
}

// FIXME the code is the same as for .block:
static auto parse_import_directive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    ++i;  // .import:

    auto attributes = std::set<std::string>{};
    ++i;  // opening of the attribute list "[["

    if (tokens.at(i) != "]]") {
        // first attribute
        attributes.insert(tokens.at(i++).str());

        // all the rest
        while (tokens.at(i) != "]]") {
            // the "," between attributes
            ++i;

            // the attribute
            attributes.insert(tokens.at(i++).str());
        }
    }

    ++i;  // closing of the attribute list "]]"

    auto frag = std::make_unique<Import_directive>(tokens.at(i).str()  // name
                                                   ,
                                                   std::move(attributes));
    frag->add(tokens.at(0));
    frag->add(tokens.at(i++));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_mark_directive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Mark_directive>(tokens.at(1).str());
    frag->add(tokens.at(i++));  // .mark:
    frag->add(tokens.at(i++));  // <key>

    fragments.push_back(std::move(frag));

    return i;
}

// FIXME this is duplicated code
static auto string_to_opcode(std::string const& s) -> std::optional<OPCODE> {
    for (auto const& [opcode, name] : OP_NAMES) {
        if (name == s) {
            return opcode;
        }
    }
    return {};
}

static auto string_to_register_set(std::string const& s)
    -> viua::internals::Register_sets {
    using viua::internals::Register_sets;
    static auto const mapping = std::map<std::string, Register_sets>{
        {
            "local",
            Register_sets::LOCAL,
        },
        {
            "static",
            Register_sets::STATIC,
        },
        {
            "global",
            Register_sets::GLOBAL,
        },
        {
            "arguments",
            Register_sets::ARGUMENTS,
        },
        {
            "parameters",
            Register_sets::PARAMETERS,
        },
        {
            "closure_local",
            Register_sets::CLOSURE_LOCAL,
        },
    };
    return mapping.at(s);
}

static auto string_to_access_type(std::string const& s)
    -> viua::internals::Access_specifier {
    using viua::internals::Access_specifier;
    static auto const mapping = std::map<std::string, Access_specifier>{
        {
            "%",
            Access_specifier::DIRECT,
        },
        {
            "@",
            Access_specifier::REGISTER_INDIRECT,
        },
        {
            "*",
            Access_specifier::POINTER_DEREFERENCE,
        },
    };
    return mapping.at(s);
}

static auto parse_any_no_input_instruction(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i).str()).value());
    frag->add(tokens.at(i++));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_register_address(
    Instruction& instruction,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    using viua::tooling::libs::lexer::classifier::is_id;

    auto const is_iota = (tokens.at(1) == "iota");
    auto const is_name = is_id(tokens.at(1).str());
    auto const index   = static_cast<viua::internals::types::register_index>(
        (is_iota or is_name) ? 0 : std::stoul(tokens.at(1).str()));
    auto const register_set = string_to_register_set(tokens.at(2).str());
    auto access             = string_to_access_type(tokens.at(0).str());

    auto operand = std::make_unique<Register_address>(
        index, is_iota, is_name, register_set, access);
    operand->add(tokens.at(0));
    operand->add(tokens.at(1));
    operand->add(tokens.at(2));

    instruction.operands.push_back(std::move(operand));

    instruction.add(tokens.at(0));
    instruction.add(tokens.at(1));
    instruction.add(tokens.at(2));

    return 3;
}

static auto parse_void(
    Instruction& instruction,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto operand = std::make_unique<Void>();
    operand->add(tokens.at(0));

    instruction.operands.push_back(std::move(operand));
    instruction.add(tokens.at(0));

    return 1;
}

static auto parse_function_name(
    Instruction& instruction,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto name = std::make_unique<Function_name>(
        tokens.at(i).str()  // name
        ,
        std::stoull(tokens.at(i + 2).str())  // arity
    );
    name->add(tokens.at(i++));
    name->add(tokens.at(i++));
    name->add(tokens.at(i++));

    instruction.operands.push_back(std::move(name));

    return i;
}

static auto parse_any_1_register_instruction(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(1));

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_2_register_instruction(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_3_register_instruction(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_3_register_with_void_target_instruction(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    } else {
        i += parse_register_address(*frag, tokens.advance(i));
    }
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_4_register_instruction(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_vinsert(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    } else {
        i += parse_register_address(*frag, tokens.advance(i));
    }

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_vpop(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_void;

    if (auto const& token = tokens.at(i); is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    } else {
        i += parse_register_address(*frag, tokens.advance(i));
    }

    i += parse_register_address(*frag, tokens.advance(i));

    if (auto const& token = tokens.at(i); is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    } else {
        i += parse_register_address(*frag, tokens.advance(i));
    }

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_integer(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit =
        std::make_unique<Integer_literal>(std::stoll(tokens.at(i).str()));
    lit->add(tokens.at(i));

    frag->operands.push_back(std::move(lit));
    frag->add(tokens.at(i++));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_float(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Float_literal>(std::stod(tokens.at(i).str()));
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_text(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Text_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_atom(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Atom_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_bits(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(1));

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else {
        auto lit = std::make_unique<Bits_literal>(tokens.at(i).str());
        lit->add(tokens.at(i++));
        frag->operands.push_back(std::move(lit));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_vector(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));

    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    } else {
        i += parse_register_address(*frag, tokens.advance(i));
    }

    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    } else {
        i += parse_register_address(*frag, tokens.advance(i));
    }

    for (auto j = index_type{0}; j < i; ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto string_to_boolean(std::string const& s) -> bool {
    if (s == "true") {
        return true;
    } else if (s == "false") {
        return false;
    } else {
        throw std::domain_error{s};
    }
}

static auto parse_op_bitset(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    auto lit = std::make_unique<Boolean_literal>(
        string_to_boolean(tokens.at(i).str()));
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_call(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    }

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        i += parse_function_name(*frag, tokens.advance(i));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register address, or function name"});
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_function(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));
    i += parse_register_address(*frag, tokens.advance(i));

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = tokens.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        i += parse_function_name(*frag, tokens.advance(i));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected function name"});
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_tailcall(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(1));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        i += parse_function_name(*frag, tokens.advance(1));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_join(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    }

    i += parse_register_address(*frag, tokens.advance(i));

    auto lit = std::make_unique<Timeout_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_receive(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    }

    auto lit = std::make_unique<Timeout_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_jump_target(
    Instruction& instruction,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_id;
    if (auto const& token = tokens.at(0); is_id(token.str())) {
        auto label = std::make_unique<Jump_label>(token.str());
        label->add(tokens.at(i++));
        instruction.operands.push_back(std::move(label));
    } else if (auto const c = token.str().at(0);
               (c == '+' or c == '-')
               and is_decimal_integer(token.str().substr(1))) {
        auto offset = std::make_unique<Jump_offset>(token.str());
        offset->add(tokens.at(i++));
        instruction.operands.push_back(std::move(offset));
    }

    return i;
}

static auto parse_op_jump(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_jump_target(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_if(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_jump_target(*frag, tokens.advance(i));
    i += parse_jump_target(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_catch(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    auto type_name = std::make_unique<Text_literal>(tokens.at(i).str());
    type_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(type_name));

    auto block_name = std::make_unique<Block_name>(tokens.at(i).str());
    block_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(block_name));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_enter(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    auto block_name = std::make_unique<Block_name>(tokens.at(i).str());
    block_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(block_name));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_import(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());

    auto module_name = std::make_unique<Module_name>(tokens.at(i).str());
    module_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(module_name));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_io_wait(
    std::vector<std::unique_ptr<Fragment>>& fragments,
    vector_view<viua::tooling::libs::lexer::Token> const& tokens)
    -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(
        string_to_opcode(tokens.at(i++).str()).value());
    frag->add(tokens.at(0));

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i);
        is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    }

    i += parse_register_address(*frag, tokens.advance(i));

    auto lit = std::make_unique<Timeout_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

// FIXME this is duplicated code
static auto make_unexpected_token_error(
    viua::tooling::libs::lexer::Token const& token,
    std::string message) -> viua::tooling::errors::compile_time::Error {
    return viua::tooling::errors::compile_time::Error{
        ((token == "\n") ? viua::tooling::errors::compile_time::
                 Compile_time_error::Unexpected_newline
                         : viua::tooling::errors::compile_time::
                             Compile_time_error::Unexpected_token),
        token,
        std::move(message)}
        .note("got "
              + viua::util::string::ops::quoted(
                  (viua::util::string::ops::strencode(token.str()))));
}

auto parse(std::vector<viua::tooling::libs::lexer::Token> const& tokens)
    -> std::vector<std::unique_ptr<Fragment>> {
    auto fragments = std::vector<std::unique_ptr<Fragment>>{};

    /*
     * Skip the newline after every fragment. Newline characters act
     * as fragment terminators.
     */
    for (auto i = index_type{0}; i < tokens.size(); ++i) {
        auto const& token = tokens.at(i);

        auto const opcode = string_to_opcode(token.str());
        if (opcode.has_value()) {
            switch (opcode.value()) {
            case NOP:
            case TRY:
            case LEAVE:
            case RETURN:
            case HALT:
                i += parse_any_no_input_instruction(fragments,
                                                    vector_view{tokens, i});
                break;
            case IZERO:
            case IINC:
            case IDEC:
            case WRAPINCREMENT:
            case WRAPDECREMENT:
            case CHECKEDSINCREMENT:
            case CHECKEDSDECREMENT:
            case CHECKEDUINCREMENT:
            case CHECKEDUDECREMENT:
            case SATURATINGSINCREMENT:
            case SATURATINGSDECREMENT:
            case SATURATINGUINCREMENT:
            case SATURATINGUDECREMENT:
            case DELETE:
            case PRINT:
            case ECHO:
            case FRAME:
            case ALLOCATE_REGISTERS:
            case SELF:
            case THROW:
            case DRAW:
            case STRUCT:
                i += parse_any_1_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case NOT:
            case ITOF:
            case FTOI:
            case STOI:
            case STOF:
            case TEXTLENGTH:
            case VPUSH:
            case VLEN:
            case BITNOT:
            case BITSWIDTH:
            case ROL:
            case ROR:
            case MOVE:
            case COPY:
            case PTR:
            case PTRLIVE:
            case SWAP:
            case ISNULL:
            case SEND:
            case STRUCTKEYS:
            case BITS_OF_INTEGER:
            case INTEGER_OF_BITS:
                i += parse_any_2_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case VINSERT:
                i += parse_op_vinsert(fragments, vector_view{tokens, i});
                break;
            case VPOP:
                i += parse_op_vpop(fragments, vector_view{tokens, i});
                break;
            case ADD:
            case SUB:
            case MUL:
            case DIV:
            case LT:
            case LTE:
            case GT:
            case GTE:
            case EQ:
            case PIDEQ:
            case TEXTEQ:
            case TEXTAT:
            case TEXTCOMMONPREFIX:
            case TEXTCOMMONSUFFIX:
            case TEXTCONCAT:
            case VAT:
            case AND:
            case OR:
            case BITAND:
            case BITOR:
            case BITXOR:
            case BITAT:
            case SHL:
            case SHR:
            case ASHL:
            case ASHR:
            case WRAPADD:
            case WRAPSUB:
            case WRAPMUL:
            case WRAPDIV:
            case CHECKEDSADD:
            case CHECKEDSSUB:
            case CHECKEDSMUL:
            case CHECKEDSDIV:
            case CHECKEDUADD:
            case CHECKEDUSUB:
            case CHECKEDUMUL:
            case CHECKEDUDIV:
            case SATURATINGSADD:
            case SATURATINGSSUB:
            case SATURATINGSMUL:
            case SATURATINGSDIV:
            case SATURATINGUADD:
            case SATURATINGUSUB:
            case SATURATINGUMUL:
            case SATURATINGUDIV:
            case CAPTURE:
            case CAPTURECOPY:
            case CAPTUREMOVE:
            case ATOMEQ:
            case STRUCTINSERT:
                i += parse_any_3_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case STRUCTREMOVE:
                i += parse_any_3_register_with_void_target_instruction(
                    fragments, vector_view{tokens, i});
                break;
            case STRUCTAT:
                i += parse_any_3_register_with_void_target_instruction(
                    fragments, vector_view{tokens, i});
                break;
            case TEXTSUB:
                i += parse_any_4_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case INTEGER:
                i += parse_op_integer(fragments, vector_view{tokens, i});
                break;
            case FLOAT:
                i += parse_op_float(fragments, vector_view{tokens, i});
                break;
            case STRING:
            case TEXT:
                i += parse_op_text(fragments, vector_view{tokens, i});
                break;
            case ATOM:
                i += parse_op_atom(fragments, vector_view{tokens, i});
                break;
            case BITS:
                i += parse_op_bits(fragments, vector_view{tokens, i});
                break;
            case VECTOR:
                i += parse_op_vector(fragments, vector_view{tokens, i});
                break;
            case BITSET:
                i += parse_op_bitset(fragments, vector_view{tokens, i});
                break;
            case CLOSURE:
            case FUNCTION:
                i += parse_op_function(fragments, vector_view{tokens, i});
                break;
            case CALL:
            case PROCESS:
                i += parse_op_call(fragments, vector_view{tokens, i});
                break;
            case TAILCALL:
            case DEFER:
            case WATCHDOG:
                i += parse_op_tailcall(fragments, vector_view{tokens, i});
                break;
            case JOIN:
                i += parse_join(fragments, vector_view{tokens, i});
                break;
            case RECEIVE:
                i += parse_receive(fragments, vector_view{tokens, i});
                break;
            case JUMP:
                i += parse_op_jump(fragments, vector_view{tokens, i});
                break;
            case IF:
                i += parse_op_if(fragments, vector_view{tokens, i});
                break;
            case CATCH:
                i += parse_op_catch(fragments, vector_view{tokens, i});
                break;
            case ENTER:
                i += parse_op_enter(fragments, vector_view{tokens, i});
                break;
            case IMPORT:
                i += parse_op_import(fragments, vector_view{tokens, i});
                break;
            case IO_READ:
            case IO_WRITE:
                i += parse_any_3_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case IO_CLOSE:
                i += parse_any_1_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case IO_WAIT:
                i += parse_op_io_wait(fragments, vector_view{tokens, i});
                break;
            case IO_CANCEL:
                i += parse_any_1_register_instruction(fragments,
                                                      vector_view{tokens, i});
                break;
            case PARAM:
            case PAMV:
            case ARG:
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        // FIXME make a special error code for internal
                        // instructions in user code
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        token,
                        "internal instruction found in user code"});
                break;
            case STREQ:
            case BOOL:
            case BITSEQ:
            case BITSLT:
            case BITSLTE:
            case BITSGT:
            case BITSGTE:
            case BITAEQ:
            case BITALT:
            case BITALTE:
            case BITAGT:
            case BITAGTE:
            default:
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        // FIXME make a special error code for unimplemented
                        // instructions
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        token,
                        ("reduction for `" + token.str()
                         + "' instruction is not implemented")});
            }

            continue;
        }

        if (token == ".extern_function:") {
            i += parse_extern_function_directive(fragments,
                                                 vector_view{tokens, i});
        } else if (token == ".extern_block:") {
            i +=
                parse_extern_block_directive(fragments, vector_view{tokens, i});
        } else if (token == ".function:") {
            i += parse_function_head<Function_head>(fragments,
                                                    vector_view{tokens, i});
        } else if (token == ".closure:") {
            i += parse_function_head<Closure_head>(fragments,
                                                   vector_view{tokens, i});
        } else if (token == ".block:") {
            i += parse_block_head(fragments, vector_view{tokens, i});
        } else if (token == ".end") {
            fragments.push_back(std::make_unique<End_directive>());
            fragments.back()->add(tokens.at(i++));
        } else if (token == ".info:") {
            i += parse_info_directive(fragments, vector_view{tokens, i});
        } else if (token == ".import:") {
            i += parse_import_directive(fragments, vector_view{tokens, i});
        } else if (token == ".name:") {
            i += parse_name_directive(fragments, vector_view{tokens, i});
        } else if (token == ".mark:") {
            i += parse_mark_directive(fragments, vector_view{tokens, i});
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
                make_unexpected_token_error(token,
                                            "cannot be reduced by any rule"));
        }
    }

    return fragments;
}

auto Cooked_function::head() const -> Function_head const& {
    return *static_cast<Function_head*>(lines.at(0).get());
}

auto Cooked_function::body() const -> body_type {
    auto v = body_type{};

    for (auto each = lines.begin() + 1; each != lines.end(); ++each) {
        v.push_back(each->get());
    }

    return v;
}

auto Cooked_block::body() const -> body_type {
    auto v = body_type{};

    for (auto each = lines.begin() + 1; each != lines.end(); ++each) {
        v.push_back(each->get());
    }

    return v;
}

Cooked_fragments::Cooked_fragments(std::string f_n)
        : file_name{std::move(f_n)} {}

auto cook(std::string file_name,
          std::vector<std::unique_ptr<Fragment>> fragments)
    -> Cooked_fragments {
    auto cooked = Cooked_fragments{std::move(file_name)};

    auto function = std::unique_ptr<Cooked_function>{};
    auto closure  = std::unique_ptr<Cooked_closure>{};
    auto block    = std::unique_ptr<Cooked_block>{};

    for (auto& each : fragments) {
        switch (each->type()) {
        case Fragment_type::Extern_function: {
            auto t = std::unique_ptr<Extern_function>{};
            t.reset(static_cast<Extern_function*>(each.release()));
            cooked.extern_function_fragments.push_back(std::move(t));
            break;
        }
        case Fragment_type::Extern_block: {
            auto t = std::unique_ptr<Extern_block>{};
            t.reset(static_cast<Extern_block*>(each.release()));
            cooked.extern_block_fragments.push_back(std::move(t));
            break;
        }
        case Fragment_type::Info_directive: {
            auto t = std::unique_ptr<Info_directive>{};
            t.reset(static_cast<Info_directive*>(each.release()));
            cooked.info_fragments.push_back(std::move(t));
            break;
        }
        case Fragment_type::Import_directive: {
            auto t = std::unique_ptr<Import_directive>{};
            t.reset(static_cast<Import_directive*>(each.release()));
            cooked.import_fragments.push_back(std::move(t));
            break;
        }
        case Fragment_type::End_directive: {
            if (function) {
                auto const& fn =
                    *static_cast<Function_head*>(function->lines.at(0).get());
                cooked.function_fragments[fn.function_name + '/'
                                          + std::to_string(fn.arity)] =
                    std::move(*function);
                function = nullptr;
            } else if (closure) {
                auto const& fn =
                    *static_cast<Closure_head*>(closure->lines.at(0).get());
                cooked.closure_fragments[fn.function_name + '/'
                                         + std::to_string(fn.arity)] =
                    std::move(*closure);
                closure = nullptr;
            } else if (block) {
                auto const& bl =
                    *static_cast<Block_head*>(block->lines.at(0).get());
                cooked.block_fragments[bl.name] = std::move(*block);
                block                           = nullptr;
            } else {
                // do nothing
            }
            break;
        }
        case Fragment_type::Function_head: {
            if (function) {
                // FIXME Use some better error, maybe Unended_function?
                auto const& fn = function->head();
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        fn.token(fn.tokens().size() - 3),
                        "function not ended"}
                                .add(fn.token(fn.tokens().size() - 2))
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else if (closure) {
                // FIXME Use some better error, maybe Unended_closure?
                auto const& fn =
                    *static_cast<Closure_head*>(closure->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(
                                fn.token(fn.tokens().size() - 3),
                                "closure not ended")
                                .add(fn.token(fn.tokens().size() - 2))
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else if (block) {
                // FIXME Use some better error, maybe Unended_block?
                auto const& fn =
                    *static_cast<Function_head*>(block->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(
                        fn.token(fn.tokens().size() - 1), "block not ended"))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else {
                function = std::make_unique<Cooked_function>();
                function->lines.push_back(std::move(each));
            }
            break;
        }
        case Fragment_type::Closure_head: {
            if (function) {
                // FIXME Use some better error, maybe Unended_function?
                auto const& fn =
                    *static_cast<Function_head*>(function->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(fn.token(0),
                                                        "function not ended")
                                .add(fn.token(fn.tokens().size() - 3))
                                .add(fn.token(fn.tokens().size() - 2))
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else if (closure) {
                // FIXME Use some better error, maybe Unended_closure?
                auto const& fn =
                    *static_cast<Closure_head*>(closure->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(fn.token(0),
                                                        "closure not ended")
                                .add(fn.token(fn.tokens().size() - 3))
                                .add(fn.token(fn.tokens().size() - 2))
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else if (block) {
                // FIXME Use some better error, maybe Unended_block?
                auto const& fn =
                    *static_cast<Function_head*>(block->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(fn.token(0),
                                                        "block not ended")
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else {
                closure = std::make_unique<Cooked_closure>();
                closure->lines.push_back(std::move(each));
            }
            break;
        }
        case Fragment_type::Block_head: {
            if (function) {
                // FIXME Use some better error, maybe Unended_function?
                auto const& fn =
                    *static_cast<Function_head*>(function->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(fn.token(0),
                                                        "function not ended")
                                .add(fn.token(fn.tokens().size() - 3))
                                .add(fn.token(fn.tokens().size() - 2))
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else if (closure) {
                // FIXME Use some better error, maybe Unended_closure?
                auto const& fn =
                    *static_cast<Closure_head*>(closure->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(fn.token(0),
                                                        "closure not ended")
                                .add(fn.token(fn.tokens().size() - 3))
                                .add(fn.token(fn.tokens().size() - 2))
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else if (block) {
                // FIXME Use some better error, maybe Unended_block?
                auto const& fn =
                    *static_cast<Function_head*>(block->lines.at(0).get());
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(fn.token(0),
                                                        "block not ended")
                                .add(fn.token(fn.tokens().size() - 1)))
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        each->token(0),
                        "expected `.end' here:"});
            } else {
                block = std::make_unique<Cooked_block>();
                block->lines.push_back(std::move(each));
            }
            break;
        }
        case Fragment_type::Instruction: {
        }
        case Fragment_type::Name_directive: {
        }
        case Fragment_type::Mark_directive: {
            if (function) {
                function->lines.push_back(std::move(each));
            } else if (closure) {
                closure->lines.push_back(std::move(each));
            } else if (block) {
                block->lines.push_back(std::move(each));
            } else {
                // FIXME Use some better error, maybe `Orphaned_instruction` for
                // instructions outside of blocks and `Out_of_context_directive`
                // for names and marks?
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(make_unexpected_token_error(
                        each->token(0),
                        "orphaned instruction or out-of-context directive"));
            }
            break;
        }
        default: {
            // FIXME Use some better error, maybe Unexpected_fragment?
            throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
                make_unexpected_token_error(each->token(0),
                                            "could not be parsed"));
        }
        }
    }

    return cooked;
}

}}}}  // namespace viua::tooling::libs::parser
