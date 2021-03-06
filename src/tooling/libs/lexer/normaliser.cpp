/*
 *  Copyright (C) 2018, 2019 Marek Marecki
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
#include <optional>
#include <string>
#include <vector>

#include <viua/bytecode/maps.h>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/lexer/classifier.h>
#include <viua/tooling/libs/lexer/normaliser.h>
#include <viua/util/string/ops.h>
#include <viua/util/vector_view.h>

namespace viua { namespace tooling { namespace libs { namespace lexer {
namespace normaliser {
using viua::util::vector_view;
using index_type = std::vector<Token>::size_type;

/*
 * All normalise_*() functions assume that they are called at the appropriate
 * moment. They return the number of tokens consumed, so that the counter in the
 * main loop can be updated like this:
 *
 *      i += normalise_fn(...);
 *
 */
static auto normalise_function_extern_function(std::vector<Token>& tokens,
                                               vector_view<Token> const& source)
    -> index_type
{
    auto i = std::remove_reference_t<decltype(source)>::size_type{0};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));  // function name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected function name"});
    }

    if (auto const& token = source.at(++i); token == "/") {
        tokens.push_back(source.at(i));  // arity separator
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected / (arity separator)"});
    }

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    if (auto const& token = source.at(++i); is_decimal_integer(token.str())) {
        tokens.push_back(source.at(i));  // arity
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected arity (decimal integer)"});
    }

    return ++i;
}

static auto normalise_directive_extern_function(
    std::vector<Token>& tokens,
    vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));
    return normalise_function_extern_function(tokens, source.advance(1)) + 1;
}

static auto normalise_directive_extern_block(std::vector<Token>& tokens,
                                             vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));  // block name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected block name"});
    }

    return ++i;
}

static auto normalise_directive_info(std::vector<Token>& tokens,
                                     vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));  // key
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected module metadata key name (id or scoped id)"});
    }

    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    if (auto const& token = source.at(++i); is_quoted_text(token.str())) {
        tokens.push_back(source.at(i));  // value
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected module metadata value (quoted string)"});
    }

    return ++i;
}

static auto normalise_attribute_list(std::vector<Token>& tokens,
                                     vector_view<Token> const& source)
    -> index_type
{
    auto i = std::remove_reference_t<decltype(source)>::size_type{0};

    if (auto const& token = source.at(i); token == "[[") {
        tokens.push_back(token);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected attribute list opening: `[[`"});
    }

    ++i;

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i); token == "]]") {
        // do nothing, we deal with it later
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        // do nothing, we deal with it later
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected id, scoped id, or attribute list closing: `]]`"});
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
            throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
                viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Unexpected_token,
                    token,
                    "expected comma, or attribute list closing: `]]`"});
        }

        if (auto const& token = source.at(++i);
            is_id(token.str()) or is_scoped_id(token.str())) {
            tokens.push_back(token);
        } else {
            throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
                viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Unexpected_token,
                    token,
                    "expected id, or scoped id"});
        }
    }

    /*
     * Push the closing ']]'.
     */
    tokens.push_back(source.at(i));

    return ++i;
}

static auto normalise_directive_import(std::vector<Token>& tokens,
                                       vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(
            Token{token.line(), token.character(), "[[", token.str()});
        tokens.push_back(
            Token{token.line(), token.character(), "]]", token.str()});
    }

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));  // module name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected module name"});
    }

    return ++i;
}

static auto normalise_directive_mark(std::vector<Token>& tokens,
                                     vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));  // marker name
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected marker name"});
    }

    return ++i;
}

static auto normalise_directive_name(std::vector<Token>& tokens,
                                     vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    // FIXME maybe add a function that will check tokens for iota-iness?
    if (auto const& token = source.at(i);
        is_decimal_integer(token.str()) or token == "iota") {
        tokens.push_back(source.at(i++));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register index"});
    }

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i++));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected id or scoped id"});
    }

    return i;
}

/*
 * Creates an error, with appropriate type, depending on whether the
 * token supplied is a newline or not.
 */
static auto make_unexpected_token_error(Token const& token, std::string message)
    -> viua::tooling::errors::compile_time::Error
{
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

static auto normalise_register_access(std::vector<Token>& tokens,
                                      vector_view<Token> const& source)
    -> index_type
{
    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(0);
        is_access_type_specifier(token.str())) {
        tokens.push_back(token);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token,
                                        "expected register access specifier"));
    }

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_id;  // registers may have
                                                          // user-defined names,
                                                          // so we have to allow
                                                          // ids here (but not
                                                          // scoped ones!)
    if (auto const& register_index = source.at(1);
        is_decimal_integer(register_index.str())
        or is_id(register_index.str())) {
        tokens.push_back(register_index);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(
                register_index, "expected register index (decimal integer)"));
    }

    using viua::tooling::libs::lexer::classifier::is_register_set_name;
    if (auto const& register_set = source.at(2);
        is_register_set_name(register_set.str())) {
        tokens.push_back(register_set);
    } else {
        auto e = make_unexpected_token_error(register_set,
                                             "expected register set specifier");
        if (register_set.str().empty() or (register_set.str().at(0) == '\n')) {
            e.aside(source.at(2), "register set specifier missing here");
        } else {
            auto const likeness_limit =
                viua::util::string::ops::LevenshteinDistance{4};
            auto const best_match = viua::util::string::ops::levenshtein_best(
                register_set.str(),
                {// FIXME provide a std::vector<std::string> with valid register
                 // set names
                 "local",
                 "static",
                 "global",
                 "arguments",
                 "parameters"},
                likeness_limit);
            if (best_match.first <= likeness_limit) {
                e.aside(source.at(2),
                        "did you mean `" + best_match.second + "'?");
            }
        }
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(e);
    }

    return 3;
}

static auto normalise_ctor_target_register_access(
    std::vector<Token>& tokens,
    vector_view<Token> const& source) -> index_type
{
    if (auto const& access_specifier = source.at(0); access_specifier != "%") {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Invalid_access_type_specifier,
                access_specifier,
                "expected direct register access specifier"}
                .aside("expected \"%\""));
    }

    tokens.push_back(source.at(0));

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_id;  // registers may have
                                                          // user-defined names,
                                                          // so we have to allow
                                                          // ids here (but not
                                                          // scoped ones!)
    if (auto const& register_index = source.at(1);
        is_decimal_integer(register_index.str())
        or is_id(register_index.str())) {
        tokens.push_back(register_index);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                register_index,
                "expected register index (decimal integer)"});
    }

    using viua::tooling::libs::lexer::classifier::is_register_set_name;
    if (auto const& register_set = source.at(2);
        is_register_set_name(register_set.str())) {
        tokens.push_back(register_set);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                register_set,
                "expected register set specifier"});
    }

    return 3;
}

static auto normalise_jump_target(std::vector<Token>& tokens,
                                  vector_view<Token> const& source)
    -> index_type
{
    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_id;
    if (auto const& token = source.at(0); is_id(token.str())) {
        tokens.push_back(token);
    } else if (auto const c = token.str().at(0);
               (c == '+' or c == '-')
               and is_decimal_integer(token.str().substr(1))) {
        tokens.push_back(token);
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected jump target"});
    }

    return 1;
}

static auto normalise_call(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else if (token == "void") {
        tokens.push_back(token);
        ++i;
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        // normalise the token stream by inserting a return-value-specifier
        // token
        tokens.push_back(Token{token.line(), token.character(), "void"});
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier, `void`, or function "
                "name"});
    }

    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(i));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // function name
        tokens.push_back(source.at(++i));  // arity separator
        tokens.push_back(source.at(++i));  // arity
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier, `void`, or function "
                "name"});
    }

    return i;
}

static auto normalise_tailcall(std::vector<Token>& tokens,
                               vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else if (is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(source.at(i));    // function name
        tokens.push_back(source.at(++i));  // arity separator
        tokens.push_back(source.at(++i));  // arity
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register address, or function name"});
    }

    return i;
}

static auto normalise_allocate_registers(std::vector<Token>& tokens,
                                         vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        not is_access_type_specifier(token.str())) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier for local register set"});
    }

    if (auto const& token = source.at(i); token.str() == "%") {
        i += normalise_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Invalid_access_type_specifier,
                token,
                "expected direct register access specifier"}
                .aside("expected \"%\""));
    }

    return i;
}

static auto normalise_text(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    if (auto const& token = source.at(i); is_quoted_text(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(
            Token{token.line(), token.character(), "\"\"", token.str()});
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "`" + token.str() + "': expected quoted text"});
    }

    return i;
}

static auto normalise_atom(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    if (auto const& token = source.at(i); is_quoted_text(token.str(), '\'')) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(
            Token{token.line(), token.character(), "''", token.str()});
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "`" + token.str() + "': expected atom (single quoted text)"});
    }

    return i;
}

static auto normalise_izero(std::vector<Token>& tokens,
                            vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));
    return normalise_ctor_target_register_access(tokens, source.advance(1)) + 1;
}

static auto normalise_float(std::vector<Token>& tokens,
                            vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_ctor_target_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_float;
    if (auto const& token = source.at(i); is_float(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(
            Token{token.line(), token.character(), "0.0", token.str()});
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected floating point literal"});
    }

    return i;
}

static auto normalise_integer(std::vector<Token>& tokens,
                              vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_ctor_target_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_decimal_integer;
    using viua::tooling::libs::lexer::classifier::is_default;
    if (auto const& token = source.at(i); is_decimal_integer(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(
            Token{token.line(), token.character(), "0", token.str()});
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected decimal integer literal"});
    }

    return i;
}

static auto normalise_bits(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_ctor_target_register_access(tokens, source.advance(1));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_binary_integer;
    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_hexadecimal_integer;
    using viua::tooling::libs::lexer::classifier::is_octal_integer;
    if (auto const& token = source.at(i); is_binary_integer(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_octal_integer(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_hexadecimal_integer(token.str())) {
        tokens.push_back(token);
        ++i;
    } else if (is_default(token.str())) {
        tokens.push_back(
            Token{token.line(), token.character(), "0x00", token.str()});
        ++i;
    } else if (is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(i));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected bits literal")
                .comment("binary literal:      0b01")
                .comment("octal literal:       0o01234567")
                .comment("hexadeciaml literal: 0x0123456789abcdef"));
    }

    return i;
}

static auto normalise_vector(std::vector<Token>& tokens,
                             vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    i += normalise_register_access(tokens, source.advance(i));
    if (auto const& token = source.at(i); token.str() == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        i += normalise_register_access(tokens, source.advance(i));
    }
    if (auto const& token = source.at(i); token.str() == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        i += normalise_register_access(tokens, source.advance(i));
    }

    return i;
}

static auto normalise_join(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    using viua::tooling::libs::lexer::classifier::is_void;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(i));
    } else if (is_void(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token,
                                        "expected register address or void"));
    }

    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(i));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected register address"));
    }

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_timeout_literal;
    if (auto const& token = source.at(i);
        is_timeout_literal(token.str()) or is_default(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected timeout literal")
                .comment("example timeout duration: `1s', `2500ms'"));
    }

    return i;
}

static auto normalise_receive(std::vector<Token>& tokens,
                              vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else if (token == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(
                token, "expected register access specifier or void"));
    }

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_timeout_literal;
    if (auto const& token = source.at(i);
        is_timeout_literal(token.str()) or is_default(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected timeout literal")
                .comment("example timeout duration: `1s', `2500ms'"));
    }

    return i;
}

static auto normalise_bit_set(std::vector<Token>& tokens,
                              vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_ctor_target_register_access(tokens, source.advance(i));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_ctor_target_register_access(tokens, source.advance(i));
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected register access specifier"});
    }

    using viua::tooling::libs::lexer::classifier::is_boolean_literal;
    if (auto const& token = source.at(i); is_boolean_literal(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(
                token, "expected boolean literal (`true' or `false')"));
    }

    return i;
}

static auto normalise_catch(std::vector<Token>& tokens,
                            vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    if (auto const& token = source.at(i); is_quoted_text(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected quoted type to catch")
                .comment("valid catch: catch \"A_type\" block_name"));
    }

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(
                token, "expected block name (id, or scoped id)"));
    }

    return i;
}

static auto normalise_enter(std::vector<Token>& tokens,
                            vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(
                token, "expected block name (id, or scoped id)"));
    }

    return i;
}

static auto normalise_import(std::vector<Token>& tokens,
                             vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_quoted_text;
    if (auto const& token = source.at(i); is_quoted_text(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected quoted type to catch")
                .comment("valid catch: catch \"A_type\" block_name"));
    }

    return i;
}

static auto normalise_jump(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));
    return normalise_jump_target(tokens, source.advance(1)) + 1;
}

static auto normalise_if(std::vector<Token>& tokens,
                         vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_jump_target(tokens, source.advance(i));
    i += normalise_jump_target(tokens, source.advance(i));

    return i;
}

static auto normalise_frame(std::vector<Token>& tokens,
                            vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    try {
        i += normalise_register_access(tokens, source.advance(i));
    } catch (viua::tooling::errors::compile_time::Error_wrapper& e) {
        e.errors().back().note("expected `arguments' register set");
        throw;
    }

    if (auto const& token = source.at(i - 1); token != "arguments") {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected `arguments' register set"});
    }

    return i;
}

static auto normalise_any_1_register_instruction(
    std::vector<Token>& tokens,
    vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    i += normalise_register_access(tokens, source.advance(i));

    return i;
}

static auto normalise_any_2_register_instruction(
    std::vector<Token>& tokens,
    vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));

    return i;
}

static auto normalise_any_3_register_instruction(
    std::vector<Token>& tokens,
    vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token.str() == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        i += normalise_register_access(tokens, source.advance(i));
    }
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));

    return i;
}

static auto normalise_any_4_register_instruction(
    std::vector<Token>& tokens,
    vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));

    return i;
}

static auto normalise_vinsert(std::vector<Token>& tokens,
                              vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    i += normalise_register_access(tokens, source.advance(i));
    i += normalise_register_access(tokens, source.advance(i));
    if (auto const& token = source.at(i); token.str() == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        i += normalise_register_access(tokens, source.advance(i));
    }

    return i;
}

static auto normalise_vpop(std::vector<Token>& tokens,
                           vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token.str() == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        i += normalise_register_access(tokens, source.advance(i));
    }

    i += normalise_register_access(tokens, source.advance(i));

    if (auto const& token = source.at(i); token.str() == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        i += normalise_register_access(tokens, source.advance(i));
    }

    return i;
}

static auto normalise_io_wait(std::vector<Token>& tokens,
                              vector_view<Token> const& source) -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    using viua::tooling::libs::lexer::classifier::is_access_type_specifier;
    if (auto const& token = source.at(i);
        is_access_type_specifier(token.str())) {
        i += normalise_register_access(tokens, source.advance(1));
    } else if (token == "void") {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(
                token, "expected register access specifier or void"));
    }

    i += normalise_register_access(tokens, source.advance(i));

    using viua::tooling::libs::lexer::classifier::is_default;
    using viua::tooling::libs::lexer::classifier::is_timeout_literal;
    if (auto const& token = source.at(i);
        is_timeout_literal(token.str()) or is_default(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            make_unexpected_token_error(token, "expected timeout literal")
                .comment("example timeout duration: `1s', `2500ms'"));
    }

    return i;
}

static auto normalise_closure_definition(std::vector<Token>& tokens,
                                         vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(
            Token{token.line(), token.character(), "[[", token.str()});
        tokens.push_back(
            Token{token.line(), token.character(), "]]", token.str()});
    }

    i += normalise_function_extern_function(tokens, vector_view{source, i});

    return i;
}

static auto normalise_function_definition(std::vector<Token>& tokens,
                                          vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(
            Token{token.line(), token.character(), "[[", token.str()});
        tokens.push_back(
            Token{token.line(), token.character(), "]]", token.str()});
    }

    i += normalise_function_extern_function(tokens, vector_view{source, i});

    return i;
}

static auto normalise_block_definition(std::vector<Token>& tokens,
                                       vector_view<Token> const& source)
    -> index_type
{
    tokens.push_back(source.at(0));

    auto i = std::remove_reference_t<decltype(source)>::size_type{1};

    if (auto const& token = source.at(i); token == "[[") {
        i += normalise_attribute_list(tokens, source.advance(1));
    } else {
        tokens.push_back(
            Token{token.line(), token.character(), "[[", token.str()});
        tokens.push_back(
            Token{token.line(), token.character(), "]]", token.str()});
    }

    using viua::tooling::libs::lexer::classifier::is_id;
    using viua::tooling::libs::lexer::classifier::is_scoped_id;
    if (auto const& token = source.at(i);
        is_id(token.str()) or is_scoped_id(token.str())) {
        tokens.push_back(token);
        ++i;
    } else {
        throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
            viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected id, or scoped id"});
    }

    return i;
}

static auto string_to_opcode(std::string const& s) -> std::optional<OPCODE>
{
    for (auto const& [opcode, name] : OP_NAMES) {
        if (name == s) {
            return opcode;
        }
    }
    return {};
}

auto normalise(std::vector<Token> source) -> std::vector<Token>
{
    auto tokens = std::vector<Token>{};

    for (auto i = index_type{0}; i < source.size();) {
        auto const& token = source.at(i);

        if (token == "\n") {
            /*
             * Push at most one newline at a time. This simplifies later
             * stages of the assembler.
             */
            if (tokens.empty() or tokens.back() != "\n") {
                tokens.push_back(token);
            }
            ++i;
            continue;
        }

        auto const opcode = string_to_opcode(token.str());
        if (opcode.has_value()) {
            switch (opcode.value()) {
            case NOP:
            case TRY:
            case LEAVE:
            case RETURN:
            case HALT:
                tokens.push_back(token);
                ++i;
                break;
            case IZERO:
            case STRUCT:
                i += normalise_izero(tokens, vector_view{source, i});
                break;
            case INTEGER:
                i += normalise_integer(tokens, vector_view{source, i});
                break;
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
            case SELF:
            case THROW:
            case DRAW:
                i += normalise_any_1_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case FLOAT:
                i += normalise_float(tokens, vector_view{source, i});
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
                i += normalise_any_2_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case VINSERT:
                i += normalise_vinsert(tokens, vector_view{source, i});
                break;
            case VPOP:
                i += normalise_vpop(tokens, vector_view{source, i});
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
            case STRUCTREMOVE:
            case STRUCTAT:
                i += normalise_any_3_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case STRING:
            case TEXT:
                i += normalise_text(tokens, vector_view{source, i});
                break;
            case TEXTSUB:
                i += normalise_any_4_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case BITS:
                i += normalise_bits(tokens, vector_view{source, i});
                break;
            case VECTOR:
                i += normalise_vector(tokens, vector_view{source, i});
                break;
            case BITSET:
                i += normalise_bit_set(tokens, vector_view{source, i});
                break;
            case CLOSURE:
            case FUNCTION:
            case CALL:
            case PROCESS:
                i += normalise_call(tokens, vector_view{source, i});
                break;
            case FRAME:
                i += normalise_frame(tokens, vector_view{source, i});
                break;
            case TAILCALL:
            case DEFER:
            case WATCHDOG:
                i += normalise_tailcall(tokens, vector_view{source, i});
                break;
            case ALLOCATE_REGISTERS:
                i += normalise_allocate_registers(tokens,
                                                  vector_view{source, i});
                break;
            case JOIN:
                i += normalise_join(tokens, vector_view{source, i});
                break;
            case RECEIVE:
                i += normalise_receive(tokens, vector_view{source, i});
                break;
            case JUMP:
                i += normalise_jump(tokens, vector_view{source, i});
                break;
            case IF:
                i += normalise_if(tokens, vector_view{source, i});
                break;
            case CATCH:
                i += normalise_catch(tokens, vector_view{source, i});
                break;
            case ENTER:
                i += normalise_enter(tokens, vector_view{source, i});
                break;
            case IMPORT:
                i += normalise_import(tokens, vector_view{source, i});
                break;
            case ATOM:
                i += normalise_atom(tokens, vector_view{source, i});
                break;
            case EXCEPTION:
                i += normalise_any_3_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case EXCEPTION_TAG:
            case EXCEPTION_VALUE:
                i += normalise_any_2_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case IO_READ:
            case IO_WRITE:
                i += normalise_any_3_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case IO_CLOSE:
                i += normalise_any_1_register_instruction(
                    tokens, vector_view{source, i});
                break;
            case IO_WAIT:
                i += normalise_io_wait(tokens, vector_view{source, i});
                break;
            case IO_CANCEL:
                i += normalise_any_1_register_instruction(
                    tokens, vector_view{source, i});
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
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        token,
                        "unimplemented or deprecated instruction found in user "
                        "code"});
            default:
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        token,
                        "internal instruction found in user code"});
            }

            if (i < source.size() and source.at(i) != "\n") {
                throw viua::tooling::errors::compile_time::Error_wrapper{}
                    .append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::
                            Compile_time_error::Unexpected_token,
                        source.at(i),
                        "expected newline before"});
            }

            continue;
        }

        if (token == ".extern_function:") {
            i += normalise_directive_extern_function(tokens,
                                                     vector_view{source, i});
        } else if (token == ".extern_block:") {
            i += normalise_directive_extern_block(tokens,
                                                  vector_view{source, i});
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
        } else if (token == ".name:") {
            i += normalise_directive_name(tokens, vector_view{source, i});
        } else {
            auto e = viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Unexpected_token,
                token,
                "expected a directive or an instruction"};

            {
                auto valid_expected_tokens = std::vector<std::string>{};
                for (auto const& each : OP_NAMES) {
                    valid_expected_tokens.push_back(each.second);
                }

                auto const likeness_limit =
                    viua::util::string::ops::LevenshteinDistance{4};
                auto const best_match =
                    viua::util::string::ops::levenshtein_best(
                        token.str(), valid_expected_tokens, likeness_limit);

                if (best_match.first <= likeness_limit) {
                    e.aside(token, "did you mean `" + best_match.second + "'?");
                }
            }

            throw viua::tooling::errors::compile_time::Error_wrapper{}.append(
                e);
        }
    }

    return tokens;
}
}}}}}  // namespace viua::tooling::libs::lexer::normaliser
