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
#include <string>
#include <vector>
#include <viua/bytecode/maps.h>
#include <viua/util/vector_view.h>
#include <viua/util/string/ops.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/lexer/tokenise.h>
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

    for (auto i = index_type{0}; i < tokens.size();) {
        auto const& token = tokens.at(i);

        if (token == ".signature:") {
            i += parse_signature_directive(fragments, vector_view{tokens, i});
        } else if (token == ".bsignature:") {
            i += parse_bsignature_directive(fragments, vector_view{tokens, i});
        } else if (token == ".closure:") {
            i += parse_function_head(fragments, vector_view{tokens, i});
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}
                .append(make_unexpected_token_error(token
                    , "expected a directive or an instruction"
                ));
        }

        /*
         * Skip the newline after every fragment. Newline characters act
         * as fragment terminators.
         */
        ++i;
    }

    return fragments;
}

}}}}
