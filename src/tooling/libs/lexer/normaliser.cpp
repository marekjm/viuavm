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
#include <viua/util/string/ops.h>
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
static auto normalise_function_signature(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    auto i = std::remove_reference_t<decltype(source)>::size_type{0};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // function name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected function name"
            });
    }

    if (auto const& token = source.at(++i); token == "/") {
        tokens.push_back(source.at(i));  // arity separator
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected / (arity separator)"
            });
    }

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    if (auto const& token = source.at(++i); is_decimal_integer(token.str())) {
        tokens.push_back(source.at(i));  // arity
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected arity (decimal integer)"
            });
    }

    return ++i;
}

static auto normalise_directive_signature(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));
    return normalise_function_signature(tokens, source.advance(1)) + 1;
}

static auto normalise_directive_bsignature(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // block name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected block name"
            });
    }

    return ++i;
}

static auto normalise_directive_info(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // key
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected module metadata key name (id or scoped id)"
            });
    }

    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    if (auto const& token = source.at(++i); is_quoted_text(token.str())) {
        tokens.push_back(source.at(i));    // value
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected module metadata value (quoted string)"
            });
    }

    return ++i;
}

static auto normalise_directive_import(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // module name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected module name"
            });
    }

    return ++i;
}

static auto normalise_directive_mark(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // marker name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected marker name"
            });
    }

    return ++i;
}

static auto normalise_register_access(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(0); is_access_type_specifier(token.str())) {
        tokens.push_back(token);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected register access specifier"
            });
    }

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
        auto e = viua::tooling::errors::compile_time::Error{
            viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
            , register_set
            , "expected register set specifier"
        };
        auto const likeness_limit = viua::util::string::ops::LevenshteinDistance{4};
        auto const best_match = viua::util::string::ops::levenshtein_best(register_set.str(), {
            // FIXME provide a std::vector<std::string> with valid register set names
              "local"
            , "static"
            , "global"
            , "arguments"
            , "parameters"
        }, likeness_limit);
        if (best_match.first <= likeness_limit) {
            e.aside(source.at(2), "did you mean `" + best_match.second + "'?");
        }
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(e);
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

static auto normalise_jump_target(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    if (auto const& token = source.at(0); is_id(token.str())) {
        tokens.push_back(token);
    } else if (auto const c = token.str().at(0); (c == '+' or c == '-') and is_decimal_integer(token.str().substr(1))) {
        tokens.push_back(token);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected jump target"
            });
    }

    return 1;
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
        ++i;
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
    return normalise_ctor_target_register_access(tokens, source.advance(1)) + 1;
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

static auto normalise_integer(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
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

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_default;
    if (auto const& token = source.at(i); is_decimal_integer(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "0"
            , token.str()
        });
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected decimal integer literal"
            });
    }

    return i;
}

static auto normalise_iinc(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));
    return normalise_register_access(tokens, source.advance(1)) + 1;
}

static auto normalise_idec(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));
    return normalise_register_access(tokens, source.advance(1)) + 1;
}

static auto normalise_move(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));

    return i;
}

static auto normalise_jump(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));
    return normalise_jump_target(tokens, source.advance(1)) + 1;
}

static auto normalise_frame(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    try {
        i += normalise_register_access(tokens, source.advance(i));
    } catch (viua::tooling::errors::compile_time::Error_wrapper& e) {
        e.errors().back().note("expected `arguments' register set");
        throw;
    }

    if (auto const& token = source.at(i - 1); token != "arguments") {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected `arguments' register set"
            });
    }

    return i;
}

static auto normalise_attribute_list(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    auto i = std::remove_reference_t<decltype(source)>::size_type{0};

    if (auto const& token = source.at(i); token == "[[") {
        tokens.push_back(token);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected attribute list opening: `[[`"
            });
    }

    ++i;

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); token == "]]") {
        // do nothing, we deal with it later
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        // do nothing, we deal with it later
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected id, scoped id, or attribute list closing: `]]`"
            });
    }

    /*
     * Here we deal with the easy case of empty attribute list.
     */
    if (auto const& token = source.at(i); token == "]]") {
        tokens.push_back(token);
        return 2;
    }

    /*
     * Here we can push without additional checks, because we are sure
     * that what is being pushed is either an id, or a scoped id.
     * The "we deal with it later" ifs enforced this.
     */
    tokens.push_back(source.at(i));

    while (source.at(++i) != "]]") {
        if (auto const& token = source.at(i); token == ",") {
            tokens.push_back(token);
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}
                .append(viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                    , token
                    , "expected comma, or attribute list closing: `]]`"
                });
        }

        if (auto const& token = source.at(++i); is_id(token.str()) or is_scoped_id(token.str())) {
            tokens.push_back(token);
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}
                .append(viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                    , token
                    , "expected id, or scoped id"
                });
        }
    }

    /*
     * Push the closing ']]'.
     */
    tokens.push_back(source.at(i));

    return ++i;
}

static auto normalise_closure_definition(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "[["
            , token.str()
        });
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "]]"
            , token.str()
        });
    }

    i += normalise_function_signature(tokens, vector_view{source, i});

    return i;
}

static auto normalise_function_definition(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "[["
            , token.str()
        });
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "]]"
            , token.str()
        });
    }

    i += normalise_function_signature(tokens, vector_view{source, i});

    return i;
}

static auto normalise_block_definition(std::vector<Token>& tokens, vector_view<Token> const& source) -> index_type {
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "[["
            , token.str()
        });
        tokens.push_back(Token{
            token.line()
            , token.character()
            , "]]"
            , token.str()
        });
    }

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Unexpected_token
                , token
                , "expected id, or scoped id"
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
        } else if (token == "integer") {
            i += normalise_integer(tokens, vector_view{source, i});
        } else if (token == "iinc") {
            i += normalise_iinc(tokens, vector_view{source, i});
        } else if (token == "idec") {
            i += normalise_idec(tokens, vector_view{source, i});
        } else if (token == "return" or token == "leave") {
            tokens.push_back(token);
            ++i;
        } else if (token == "move" or token == "copy") {
            i += normalise_move(tokens, vector_view{source, i});
        } else if (token == "jump") {
            i += normalise_jump(tokens, vector_view{source, i});
        } else if (token == "frame") {
            i += normalise_frame(tokens, vector_view{source, i});
        } else if (token == ".signature:") {
            i += normalise_directive_signature(tokens, vector_view{source, i});
        } else if (token == ".bsignature:") {
            i += normalise_directive_bsignature(tokens, vector_view{source, i});
        } else if (token == ".closure:") {
            i += normalise_closure_definition(tokens, vector_view{source, i});
        } else if (token == ".function:") {
            i += normalise_function_definition(tokens, vector_view{source, i});
        } else if (token == ".end") {
            tokens.push_back(token);
            ++i;
        } else if (token == ".block:") {
            i += normalise_block_definition(tokens, vector_view{source, i});
        } else if (token == ".info:") {
            i += normalise_directive_info(tokens, vector_view{source, i});
        } else if (token == ".import:") {
            i += normalise_directive_import(tokens, vector_view{source, i});
        } else if (token == ".mark:") {
            i += normalise_directive_mark(tokens, vector_view{source, i});
        } else if (token == "\n") {
            tokens.push_back(token);
            ++i;
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
