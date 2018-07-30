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
 * They return the number of tokens consumed, so that the counter in the main loop can
 * be updated like this:
 *
 *      i += normalise_fn(...);
 *
 */
static auto normalise_register_access(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_id;    // registers may have user-defined names, so
                                                            // we have to allow ids here (but not scoped
                                                            // ones!)
    if (auto const& register_index = source.at(1); is_decimal_integer(register_index.str()) or is_id(register_index.str())) {
        tokens.push_back(register_index);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , register_index
                , "expected register index (decimal integer)"
            });
    }

    using viua::tooling::libs::lexer::classifier::is_register_set_name;
    if (auto const& register_set = source.at(2); is_register_set_name(register_set.str())) {
        tokens.push_back(register_set);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , register_set
                , "expected register set specifier"
            });
    }

    return 3;
}

static auto normalise_ctor_target_register_access(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    if (auto const& access_specifier = source.at(0); access_specifier != "%") {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Invalid_access_type_specifier
                , access_specifier
                , "expected direct register access specifier"
            }.aside("expected \"%\""));
    }

    tokens.push_back(source.at(0));

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_id;    // registers may have user-defined names, so
                                                            // we have to allow ids here (but not scoped
                                                            // ones!)
    if (auto const& register_index = source.at(1); is_decimal_integer(register_index.str()) or is_id(register_index.str())) {
        tokens.push_back(register_index);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , register_index
                , "expected register index (decimal integer)"
            });
    }

    using viua::tooling::libs::lexer::classifier::is_register_set_name;
    if (auto const& register_set = source.at(2); is_register_set_name(register_set.str())) {
        tokens.push_back(register_set);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , register_set
                , "expected register set specifier"
            });
    }

    return 3;
}

static auto normalise_call(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else if (token == "void") {
        tokens.push_back(token);
        ++i;
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        // normalise the token stream by inserting a return-value-specifier token
        tokens.push_back(Token{token.line(), token.character(), "void"});
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access specifier, `void`, or function name"
            });
    }

    if (auto const& token = source.at(i); is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // function name
        tokens.push_back(source.at(++i));  // arity separator
        tokens.push_back(source.at(++i));  // arity
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access specifier, `void`, or function name"
            });
    }

    return i;
}

static auto normalise_allocate_registers(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i); not is_access_type_specifier(token.str())) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access specifier for local register set"
            });
    }

    if (auto const& token = source.at(i); token.str() == "%") {
        i += normalise_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Invalid_access_type_specifier
                , token
                , "expected direct register access specifier"
            }.aside("expected \"%\""));
    }

    return i;
}

static auto normalise_text(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i); is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access specifier"
            });
    }

    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    using viua::tooling::libs::lexer::classifier::is_default;
    if (auto const& token = source.at(i); is_quoted_text(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "\"\""
            , token.str()
        });
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "`" + token.str() + "': expected quoted text"
            });
    }

    return i;
}

static auto normalise_print(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    i += normalise_register_access(tokens, source.advance(1));

    return i;
}

static auto normalise_izero(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));
    return normalise_ctor_target_register_access(tokens, source.advance(1));
}

static auto normalise_float(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i); is_access_type_specifier(token.str())) {
        i += normalise_ctor_target_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access specifier"
            });
    }

    using viua::tooling::libs::lexer::classifier::is_float;
    using viua::tooling::libs::lexer::classifier::is_default;
    if (auto const& token = source.at(i); is_float(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "0.0"
            , token.str()
        });
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected floating point literal"
            });
    }

    return i;
}

auto normalise(std::vector<Token> source) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    for (auto i = index_type{0}; i < source.size();) {
        auto const& token = source.at(i);

        if (token == "call") {
            i += normalise_call(tokens, vector_view{source, i});
        } else if (token == "allocate_registers") {
            i += normalise_allocate_registers(tokens, vector_view{source, i});
        } else if (token == "text") {
            i += normalise_text(tokens, vector_view{source, i});
        } else if (token == "print") {
            i += normalise_print(tokens, vector_view{source, i});
        } else if (token == "izero") {
            i += normalise_izero(tokens, vector_view{source, i});
        } else if (token == "float") {
            i += normalise_float(tokens, vector_view{source, i});
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}
                .append(viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                    , token
                    , "expected a directive or an instruction"
                });
        }
    }

    return tokens;
}
}}}}}
