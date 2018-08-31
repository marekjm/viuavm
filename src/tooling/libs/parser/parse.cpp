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
#include <string>
#include <vector>
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
