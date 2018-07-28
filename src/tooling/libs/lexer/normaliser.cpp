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

#include <iostream>
#include <vector>
#include <viua/util/vector_view.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/lexer/classifier.h>
#include <viua/tooling/libs/lexer/normaliser.h>

namespace viua {
namespace tooling {
namespace libs {
namespace lexer {
namespace normaliser {
using viua::util::vector_view;
using index_type = std::vector<Token>::size_type;

/*
 * All normalise_*() functions assume that they are called at the appropriate moment.
 * They return the number of tokens consumed.
 */
static auto normalise_register_access(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    std::cerr << source.at(1).str() << std::endl;

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    if (auto const& register_index = source.at(1); is_decimal_integer(register_index.str())) {
        tokens.push_back(register_index);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , register_index
                , "expected register index (decimal integer)"
            });
    }

    return 3;
}
static auto normalise_call[[maybe_unused]](std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    std::cerr << source.at(i).str() << std::endl;

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_access_type_specifier(token.str())) {
        normalise_register_access(tokens, source.advance(1));
        i += 3;
    } else if (token == "void") {
        tokens.push_back(token);
        i += 1;
    } else if (is_id(token.str())) {
        tokens.push_back(Token{token.line(), token.character(), "void"});
        tokens.push_back(token);
        tokens.push_back(source.at(i + 1));  // arity separator
        tokens.push_back(source.at(i + 2));  // arity
    } else if (is_scoped_id(token.str())) {
        tokens.push_back(Token{token.line(), token.character(), "void"});
        tokens.push_back(token);
        tokens.push_back(source.at(i + 1));  // arity separator
        tokens.push_back(source.at(i + 2));  // arity
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access, `void`, or function name"
            });
    }

    return 3;
}

auto normalise(std::vector<Token> source) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    for (auto i = index_type{0}; i < source.size(); ++i) {
        auto const& token = source.at(i);

        if (token == "call") {
            i += normalise_call(tokens, vector_view{source, i});
        } else {
            tokens.push_back(token);
        }
    }

    return tokens;
}
}}}}}
