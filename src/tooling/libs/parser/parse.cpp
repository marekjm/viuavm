/*
 *  Copyright (C) 2018 Marek Marecki
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
#include <viua/util/vector_view.h>
#include <viua/util/string/ops.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/lexer/tokenise.h>
#include <viua/tooling/libs/lexer/classifier.h>
#include <viua/tooling/libs/parser/parser.h>

namespace viua {
namespace tooling {
namespace libs {
namespace parser {
using viua::util::vector_view;
using index_type = std::vector<viua::tooling::libs::lexer::Token>::size_type;

auto Fragment::type() const -> Fragment_type {
    return fragment_type;
}

auto Fragment::add(viua::tooling::libs::lexer::Token tok) -> void {
    tokens.push_back(std::move(tok));
}

Fragment::Fragment(Fragment_type t):
    fragment_type{t}
{}

Signature_directive::Signature_directive(std::string fn, uint64_t const a):
    Fragment{Fragment_type::Signature_directive}
    , function_name{std::move(fn)}
    , arity{a}
{}

Block_signature_directive::Block_signature_directive(std::string bn):
    Fragment{Fragment_type::Block_signature_directive}
    , block_name{std::move(bn)}
{}

Closure_head::Closure_head(std::string fn, uint64_t const a, std::set<std::string> attrs):
    Fragment{Fragment_type::Closure_head}
    , function_name{std::move(fn)}
    , arity{a}
    , attributes{std::move(attrs)}
{}

auto Operand::type() const -> Operand_type {
    return operand_type;
}

auto Operand::add(viua::tooling::libs::lexer::Token t) -> void {
    tokens.push_back(t);
}

Operand::Operand(Operand_type const o):
    operand_type{o}
{}

Register_address::Register_address(
    viua::internals::types::register_index const ri
    , viua::internals::Register_sets const rs
    , viua::internals::Access_specifier const as
):
    Operand{Operand_type::Register_address}
    , index{ri}
    , register_set{rs}
    , access{as}
{}

Integer_literal::Integer_literal(viua::types::Integer::underlying_type const x):
    Operand{Operand_type::Integer_literal}
    , n{x}
{}

Float_literal::Float_literal(viua::types::Float::underlying_type const x):
    Operand{Operand_type::Float_literal}
    , n{x}
{}

Text_literal::Text_literal(std::string s):
    Operand{Operand_type::Text_literal}
    , text{std::move(s)}
{}

Atom_literal::Atom_literal(std::string s):
    Operand{Operand_type::Atom_literal}
    , text{std::move(s)}
{}

Bits_literal::Bits_literal(std::string s):
    Operand{Operand_type::Bits_literal}
    , text{std::move(s)}
{}

Boolean_literal::Boolean_literal(bool const v):
    Operand{Operand_type::Boolean_literal}
    , value{v}
{}

Void::Void():
    Operand{Operand_type::Void}
{}

Function_name::Function_name(std::string fn, uint64_t const a):
    Operand{Operand_type::Function_name}
    , function_name{std::move(fn)}
    , arity{a}
{}

Block_name::Block_name(std::string bn):
    Operand{Operand_type::Block_name}
    , name{std::move(bn)}
{}

Module_name::Module_name(std::string bn):
    Operand{Operand_type::Module_name}
    , name{std::move(bn)}
{}

Timeout_literal::Timeout_literal(std::string s):
    Operand{Operand_type::Timeout_literal}
    , value{std::move(s)}
{}

Jump_offset::Jump_offset(std::string s):
    Operand{Operand_type::Jump_offset}
    , value{std::move(s)}
{}

Jump_label::Jump_label(std::string s):
    Operand{Operand_type::Jump_label}
    , value{std::move(s)}
{}

Instruction::Instruction(OPCODE const o):
    Fragment{Fragment_type::Instruction}
    , opcode{o}
{}

static auto parse_signature_directive(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto frag = std::make_unique<Signature_directive>(
        tokens.at(1).str()
        , std::stoull(tokens.at(3).str())
    );

    frag->add(tokens.at(0));    // .signature:
    frag->add(tokens.at(1));    // name
    frag->add(tokens.at(2));    // arity separator
    frag->add(tokens.at(3));    // arity

    fragments.push_back(std::move(frag));

    return 4;
}

static auto parse_bsignature_directive(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto frag = std::make_unique<Block_signature_directive>(
        tokens.at(1).str()
    );

    frag->add(tokens.at(0));    // .bsignature:
    frag->add(tokens.at(1));    // name

    fragments.push_back(std::move(frag));

    return 2;
}

static auto parse_function_head(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    ++i;    // .function: or .closure:

    auto attributes = std::set<std::string>{};
    ++i;    // opening of the attribute list "[["

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

    ++i;    // closing of the attribute list "]]"

    auto frag = std::make_unique<Closure_head>(
        tokens.at(i).str()                      // name
        , std::stoull(tokens.at(i + 2).str())   // arity
        , std::move(attributes)
    );
    for (auto j = index_type{0}; j < (i + 2); ++j) {
        frag->add(tokens.at(j));
    }

    fragments.push_back(std::move(frag));

    return i + 3;
}

// FIXME this is duplicated code
static auto string_to_opcode(std::string const& s) -> std::optional<OPCODE> {
    for (auto const& [ opcode, name ] : OP_NAMES) {
        if (name == s) {
            return opcode;
        }
    }
    return {};
}

static auto string_to_register_set(std::string const& s) -> viua::internals::Register_sets {
    using viua::internals::Register_sets;
    static auto const mapping = std::map<std::string, Register_sets>{
        { "local", Register_sets::LOCAL, },
        { "static", Register_sets::STATIC, },
        { "global", Register_sets::GLOBAL, },
        { "arguments", Register_sets::ARGUMENTS, },
        { "parameters", Register_sets::PARAMETERS, },
        { "closure_local", Register_sets::CLOSURE_LOCAL, },
    };
    return mapping.at(s);
}

static auto string_to_access_type(std::string const& s) -> viua::internals::Access_specifier {
    using viua::internals::Access_specifier;
    static auto const mapping = std::map<std::string, Access_specifier>{
        { "%", Access_specifier::DIRECT, },
        { "@", Access_specifier::REGISTER_INDIRECT, },
        { "*", Access_specifier::POINTER_DEREFERENCE, },
    };
    return mapping.at(s);
}

static auto parse_any_no_input_instruction(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_register_address(Instruction& instruction, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto const index = static_cast<viua::internals::types::register_index>(
        std::stoul(tokens.at(1).str())
    );
    auto const register_set = string_to_register_set(tokens.at(2).str());
    auto access = string_to_access_type(tokens.at(0).str());

    auto operand = std::make_unique<Register_address>(
        index
        , register_set
        , access
    );
    operand->add(tokens.at(0));
    operand->add(tokens.at(1));
    operand->add(tokens.at(2));

    instruction.operands.push_back(std::move(operand));

    return 3;
}

static auto parse_void(Instruction& instruction, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto operand = std::make_unique<Void>();
    operand->add(tokens.at(0));

    instruction.operands.push_back(std::move(operand));

    return 1;
}

static auto parse_function_name(Instruction& instruction, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto name = std::make_unique<Function_name>(
        tokens.at(i).str()                      // name
        , std::stoull(tokens.at(i + 2).str())   // arity
    );
    name->add(tokens.at(i++));
    name->add(tokens.at(i++));
    name->add(tokens.at(i++));

    instruction.operands.push_back(std::move(name));

    return i;
}

static auto parse_any_1_register_instruction(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_2_register_instruction(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_3_register_instruction(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_any_4_register_instruction(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_register_address(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_integer(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Integer_literal>(std::stoll(tokens.at(i).str()));
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_float(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Float_literal>(std::stod(tokens.at(i).str()));
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_text(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Text_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_atom(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Atom_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_bits(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Bits_literal>(tokens.at(i).str());
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

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

static auto parse_op_bitset(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(1));
    i += parse_register_address(*frag, tokens.advance(1));

    auto lit = std::make_unique<Boolean_literal>(string_to_boolean(tokens.at(i).str()));
    lit->add(tokens.at(i++));
    frag->operands.push_back(std::move(lit));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_call(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());
    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_void(token.str())) {
        i += parse_void(*frag, tokens.advance(i));
    }

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = tokens.at(i); is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(i));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        i += parse_function_name(*frag, tokens.advance(i));
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register address, or function name"
            });
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_tailcall(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = tokens.at(i); is_access_type_specifier(token.str())) {
        i += parse_register_address(*frag, tokens.advance(1));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        i += parse_function_name(*frag, tokens.advance(1));
    }

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_join(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());
    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_access_type_specifier(token.str())) {
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

static auto parse_receive(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());
    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = tokens.at(i); is_access_type_specifier(token.str())) {
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

static auto parse_jump_target(Instruction& instruction, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    if (auto const& token = tokens.at(0); is_id(token.str())) {
        auto label = std::make_unique<Jump_label>(token.str());
        label->add(tokens.at(i++));
        instruction.operands.push_back(std::move(label));
    } else if (auto const c = token.str().at(0); (c == '+' or c == '-') and is_decimal_integer(token.str().substr(1))) {
        auto offset = std::make_unique<Jump_offset>(token.str());
        offset->add(tokens.at(i++));
        instruction.operands.push_back(std::move(offset));
    }

    return i;
}

static auto parse_op_jump(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_jump_target(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_if(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    i += parse_register_address(*frag, tokens.advance(i));
    i += parse_jump_target(*frag, tokens.advance(i));
    i += parse_jump_target(*frag, tokens.advance(i));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_catch(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    auto type_name = std::make_unique<Text_literal>(tokens.at(i).str());
    type_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(type_name));

    auto block_name = std::make_unique<Block_name>(tokens.at(i).str());
    block_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(block_name));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_enter(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    auto block_name = std::make_unique<Block_name>(tokens.at(i).str());
    block_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(block_name));

    fragments.push_back(std::move(frag));

    return i;
}

static auto parse_op_import(std::vector<std::unique_ptr<Fragment>>& fragments, vector_view<viua::tooling::libs::lexer::Token> const& tokens) -> index_type {
    auto i = index_type{0};

    auto frag = std::make_unique<Instruction>(string_to_opcode(tokens.at(i++).str()).value());

    auto module_name = std::make_unique<Module_name>(tokens.at(i).str());
    module_name->add(tokens.at(i++));
    frag->operands.push_back(std::move(module_name));

    fragments.push_back(std::move(frag));

    return i;
}

// FIXME this is duplicated code
static auto make_unexpected_token_error(viua::tooling::libs::lexer::Token const& token, std::string message) -> viua::tooling::errors::compile_time::Error {
    return viua::tooling::errors::compile_time::Error{
        ((token == "\n")
         ?  viua::tooling::errors::compile_time::Compile_time_error::Unexpected_newline
         :  viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
        )
        , token
        , std::move(message)
    }.note("got " + viua::util::string::ops::quoted((viua::util::string::ops::strencode(token.str()))));
}

auto parse(std::vector<viua::tooling::libs::lexer::Token> const& tokens) -> std::vector<std::unique_ptr<Fragment>> {
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
                    i += parse_any_no_input_instruction(fragments, vector_view{tokens, i});
                    break;
                case IZERO:
                case IINC:
                case IDEC:
                case NOT:
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
                    i += parse_any_1_register_instruction(fragments, vector_view{tokens, i});
                    break;
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
                    i += parse_any_2_register_instruction(fragments, vector_view{tokens, i});
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
                case TEXTEQ:
                case TEXTAT:
                case TEXTCOMMONPREFIX:
                case TEXTCOMMONSUFFIX:
                case TEXTCONCAT:
                case VECTOR:
                case VINSERT:
                case VPOP:
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
                case STRUCTREMOVE:
                    i += parse_any_3_register_instruction(fragments, vector_view{tokens, i});
                    break;
                case TEXTSUB:
                    i += parse_any_4_register_instruction(fragments, vector_view{tokens, i});
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
                case BITSET:
                    i += parse_op_bitset(fragments, vector_view{tokens, i});
                    break;
                case CLOSURE:
                case FUNCTION:
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
                case PARAM:
                case PAMV:
                case ARG:
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            // FIXME make a special error code for internal instructions in user code
                            viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                            , token
                            , "internal instruction found in user code"
                        });
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
                            // FIXME make a special error code for unimplemented instructions
                            viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                            , token
                            , ("reduction for `"
                                + token.str()
                                + "' instruction is not implemented"
                            )
                        });
            }

            continue;
        }

        if (token == ".signature:") {
            i += parse_signature_directive(fragments, vector_view{tokens, i});
        } else if (token == ".bsignature:") {
            i += parse_bsignature_directive(fragments, vector_view{tokens, i});
        } else if (token == ".closure:") {
            i += parse_function_head(fragments, vector_view{tokens, i});
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}
                .append(make_unexpected_token_error(token
                    , "cannot be reduced by any rule"
                ));
        }
    }

    return fragments;
}

}}}}
