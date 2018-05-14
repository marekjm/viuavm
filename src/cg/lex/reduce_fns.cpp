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

#include <map>
#include <set>
#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>
#include <viua/support/string.h>
using namespace std;

namespace viua { namespace cg { namespace lex {
static auto is_valid_register_id(std::string s) -> bool {
    return (str::isnum(s) or str::isid(s));
}
static auto match(std::vector<Token> const& tokens,
                  std::remove_reference<decltype(tokens)>::type::size_type i,
                  std::vector<std::string> const& sequence) -> bool {
    if (i + sequence.size() >= tokens.size()) {
        return false;
    }

    decltype(i) n = 0;
    while (n < sequence.size()) {
        // empty string in the sequence means "any token"
        if ((not sequence.at(n).empty())
            and tokens.at(i + n) != sequence.at(n)) {
            return false;
        }
        ++n;
    }

    return true;
}

static auto match_adjacent(
    std::vector<Token> const& tokens,
    std::remove_reference<decltype(tokens)>::type::size_type i,
    std::vector<string> const& sequence) -> bool {
    if (i + sequence.size() >= tokens.size()) {
        return false;
    }

    decltype(i) n = 0;
    while (n < sequence.size()) {
        // empty string in the sequence means "any token"
        if ((not sequence.at(n).empty())
            and tokens.at(i + n) != sequence.at(n)) {
            return false;
        }
        if (n and not adjacent(tokens.at(i + n - 1), tokens.at(i + n))) {
            return false;
        }
        ++n;
    }

    return true;
}

auto remove_spaces(std::vector<Token> input_tokens) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    std::string space(" ");
    for (auto t : input_tokens) {
        if (t.str() != space) {
            tokens.push_back(t);
        }
    }

    return tokens;
}

auto remove_comments(std::vector<Token> input_tokens) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        Token token = input_tokens.at(i);
        if (token.str() == ";") {
            // FIXME this is ugly as hell
            do {
                ++i;
            } while (i < limit and input_tokens.at(i).str() != "\n");
            if (i < limit) {
                tokens.push_back(input_tokens.at(i));
            }
        } else if (i + 2 < limit and adjacent(token, input_tokens.at(i + 1))
                   and token.str() == "-"
                   and input_tokens.at(i + 1).str() == "-") {
            // FIXME this is ugly as hell
            do {
                ++i;
            } while (i < limit and input_tokens.at(i).str() != "\n");
            if (i < limit) {
                tokens.push_back(input_tokens.at(i));
            }
        } else {
            tokens.push_back(token);
        }
    }

    return tokens;
}

auto reduce_token_sequence(std::vector<Token> input_tokens,
                           std::vector<std::string> const sequence)
    -> std::vector<Token> {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens.at(i);
        if (match_adjacent(input_tokens, i, sequence)) {
            tokens.emplace_back(
                t.line(),
                t.character(),
                join_tokens(input_tokens, i, (i + sequence.size())));
            i += (sequence.size() - 1);
            continue;
        }
        tokens.push_back(t);
    }

    return tokens;
}

auto reduce_directive(std::vector<Token> input_tokens,
                      std::string const directive) -> std::vector<Token> {
    return reduce_token_sequence(input_tokens, {".", directive, ":"});
}

auto reduce_newlines(std::vector<Token> input_tokens) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    const auto limit                    = input_tokens.size();
    decltype(input_tokens)::size_type i = 0;
    while (i < limit and input_tokens.at(i) == "\n") {
        ++i;
    }
    for (; i < limit; ++i) {
        Token token = input_tokens.at(i);
        if (token.str() == "\n") {
            tokens.push_back(token);
            while (i < limit and input_tokens.at(i).str() == "\n") {
                ++i;
            }
            --i;
        } else {
            tokens.push_back(token);
        }
    }

    return tokens;
}

auto reduce_mark_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "mark");
}

auto reduce_name_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "name");
}

auto reduce_info_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "info");
}

auto reduce_import_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "import");
}

auto reduce_function_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "function");
}

auto reduce_closure_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "closure");
}

auto reduce_end_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_token_sequence(input_tokens, {".", "end"});
}

auto reduce_signature_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "signature");
}

auto reduce_bsignature_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "bsignature");
}

auto reduce_iota_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "iota");
}

auto reduce_block_directive(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_directive(input_tokens, "block");
}

auto reduce_double_colon(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_token_sequence(input_tokens, {":", ":"});
}

auto reduce_left_attribute_bracket(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_token_sequence(input_tokens, {"[", "["});
}

auto reduce_right_attribute_bracket(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    return reduce_token_sequence(input_tokens, {"]", "]"});
}

auto reduce_function_signatures(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens.at(i);
        if (str::isid(t)) {
            auto j = i;
            while (j + 2 < limit and input_tokens.at(j + 1).str() == "::"
                   and str::isid(input_tokens.at(j + 2))
                   and adjacent(input_tokens.at(j),
                                input_tokens.at(j + 1),
                                input_tokens.at(j + 2))) {
                ++j;  // swallow "::" token
                ++j;  // swallow identifier token
            }
            if (j + 1 < limit and input_tokens.at(j + 1).str() == "/") {
                if (j + 2 < limit
                    and str::isnum(input_tokens.at(j + 2).str(), false)) {
                    if (adjacent(input_tokens.at(j),
                                 input_tokens.at(j + 1),
                                 input_tokens.at(j + 2))) {
                        ++j;  // skip "/" token
                        ++j;  // skip integer encoding arity
                    }
                } else if (j + 2 < limit
                           and input_tokens.at(j + 2).str() == "\n") {
                    if (adjacent(input_tokens.at(j), input_tokens.at(j + 1))) {
                        ++j;  // skip "/" token
                    }
                }
                if (j < limit and str::isid(input_tokens.at(j))
                    and adjacent(input_tokens.at(j - 1), input_tokens.at(j))) {
                    ++j;
                }
                tokens.emplace_back(t.line(),
                                    t.character(),
                                    join_tokens(input_tokens, i, j + 1));
                i = j;
                continue;
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

auto reduce_names(std::vector<Token> input_tokens) -> std::vector<Token> {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens.at(i);
        if (str::isid(t)) {
            auto j = i;
            while (j + 2 < limit and input_tokens.at(j + 1).str() == "::"
                   and str::isid(input_tokens.at(j + 2))
                   and adjacent(input_tokens.at(j),
                                input_tokens.at(j + 1),
                                input_tokens.at(j + 2))) {
                ++j;  // swallow "::" token
                ++j;  // swallow identifier token
            }
            if (j + 1 < limit and input_tokens.at(j + 1).str() == "/") {
                ++j;  // skip "/" token
                auto next_token = input_tokens.at(j + 1).str();
                if (j + 1 < limit
                    and (next_token != "\n" and next_token != "^"
                         and next_token != "(" and next_token != ")"
                         and next_token != "[" and next_token != "]")) {
                    if (adjacent(input_tokens.at(j), input_tokens.at(j + 1))) {
                        ++j;  // skip next token, append it to name
                    }
                }
                tokens.emplace_back(t.line(),
                                    t.character(),
                                    join_tokens(input_tokens, i, j + 1));
                i = j;
                continue;
            }
            tokens.emplace_back(
                t.line(), t.character(), join_tokens(input_tokens, i, j + 1));
            i = j;
            continue;
        }
        tokens.push_back(t);
    }

    return tokens;
}

auto reduce_offset_jumps(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens.at(i);

        if (i + 1 < limit and (t.str() == "+" or t.str() == "-")
            and str::isnum(input_tokens.at(i + 1))) {
            if (adjacent(t, input_tokens.at(i + 1))) {
                tokens.emplace_back(t.line(),
                                    t.character(),
                                    (t.str() + input_tokens.at(i + 1).str()));
                ++i;
                continue;
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

auto reduce_at_prefixed_registers(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens.at(i);

        if (i + 1 < limit and t.str() == "%"
            and input_tokens.at(i + 1).str() != "\n"
            and is_valid_register_id(input_tokens.at(i + 1))) {
            tokens.emplace_back(t.line(),
                                t.character(),
                                (t.str() + input_tokens.at(i + 1).str()));
            ++i;
            continue;
        } else if (i + 1 < limit and t.str() == "@"
                   and input_tokens.at(i + 1).str() != "\n"
                   and is_valid_register_id(input_tokens.at(i + 1))) {
            tokens.emplace_back(t.line(),
                                t.character(),
                                (t.str() + input_tokens.at(i + 1).str()));
            ++i;
            continue;
        } else if (i + 1 < limit and t.str() == "*"
                   and input_tokens.at(i + 1).str() != "\n"
                   and is_valid_register_id(input_tokens.at(i + 1))) {
            tokens.emplace_back(t.line(),
                                t.character(),
                                (t.str() + input_tokens.at(i + 1).str()));
            ++i;
            continue;
        }
        tokens.push_back(t);
    }

    return tokens;
}

auto reduce_floats(std::vector<Token> input_tokens) -> std::vector<Token> {
    decltype(input_tokens) tokens;

    const auto limit = input_tokens.size();
    for (decltype(input_tokens)::size_type i = 0; i < limit; ++i) {
        const auto t = input_tokens.at(i);
        if (str::isnum(t) and i < limit - 2 and input_tokens.at(i + 1) == "."
            and str::isnum(input_tokens.at(i + 2))) {
            if (adjacent(t, input_tokens.at(i + 1), input_tokens.at(i + 2))) {
                tokens.emplace_back(t.line(),
                                    t.character(),
                                    join_tokens(input_tokens, i, i + 3));
                ++i;  // skip "." token
                ++i;  // skip second numeric part
                continue;
            }
        }
        tokens.push_back(t);
    }

    return tokens;
}

auto move_inline_blocks_out(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    decltype(input_tokens) tokens;
    decltype(tokens) block_tokens, nested_block_tokens;
    auto opened_inside = std::string{};

    bool block_opened        = false;
    bool nested_block_opened = false;

    for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size();
         ++i) {
        auto const& token = input_tokens.at(i);

        if (token == ".block:" or token == ".function:"
            or token == ".closure:") {
            if (block_opened and token == ".block:") {
                nested_block_opened = true;
            }
            if (not nested_block_opened) {
                opened_inside = input_tokens.at(i + 1);
            }
            block_opened = true;
        }

        if (block_opened) {
            if (nested_block_opened) {
                if (nested_block_tokens.size() == 1) {
                    // name is pushed here
                    // must be mangled because we want to allow many functions
                    // to have a nested 'foo' block
                    nested_block_tokens.emplace_back(
                        token.line(),
                        token.character(),
                        (opened_inside + "__nested__" + token.str()));
                    nested_block_tokens.back().original(token);
                } else {
                    nested_block_tokens.push_back(token);
                }
            } else {
                block_tokens.push_back(token);
            }
        } else {
            tokens.push_back(token);
        }

        if (i > 0 and token == "end" and input_tokens.at(i - 1) == "\n") {
            throw Invalid_syntax(token, "missing '.' character before 'end'");
        }

        if (token == ".end") {
            if (nested_block_opened) {
                for (auto const& ntoken : nested_block_tokens) {
                    tokens.push_back(ntoken);
                }
                tokens.emplace_back(nested_block_tokens.front().line(),
                                    nested_block_tokens.front().character(),
                                    "\n");
                nested_block_opened = false;

                // Push nested block name.
                // This "substitutes" nested block body with block's name.
                block_tokens.push_back(nested_block_tokens.at(1));

                nested_block_tokens.clear();
                continue;
            }

            if (block_opened) {
                for (auto const& btoken : block_tokens) {
                    tokens.push_back(btoken);
                }
                block_opened = false;
                block_tokens.clear();
                continue;
            }
        }
    }

    return tokens;
}

static void push_unwrapped_lines(
    bool const invert,
    Token const& inner_target_token,
    std::vector<Token>& final_tokens,
    const std::vector<Token>& unwrapped_tokens,
    const std::vector<Token>& input_tokens,
    std::remove_reference<decltype(input_tokens)>::type::size_type& i) {
    const auto limit = input_tokens.size();
    if (invert) {
        final_tokens.push_back(inner_target_token);
        while (i < limit and input_tokens.at(i) != "\n") {
            final_tokens.push_back(input_tokens.at(i));
            ++i;
        }

        // this line is to push \n to the token list
        final_tokens.push_back(input_tokens.at(i++));

        for (auto to : unwrapped_tokens) {
            final_tokens.push_back(to);
        }
    } else {
        auto last_line = std::vector<Token>{};
        while (final_tokens.size()
               and final_tokens.at(final_tokens.size() - 1) != "\n") {
            last_line.insert(last_line.begin(), final_tokens.back());
            final_tokens.pop_back();
        }

        for (auto to : unwrapped_tokens) {
            final_tokens.push_back(to);
        }

        for (auto to : last_line) {
            final_tokens.push_back(to);
        }
    }
}
static void unwrap_subtokens(std::vector<Token>& unwrapped_tokens,
                             const std::vector<Token>& subtokens,
                             Token const& token) {
    for (auto subt : unwrap_lines(subtokens)) {
        unwrapped_tokens.push_back(subt);
    }
    unwrapped_tokens.emplace_back(token.line(), token.character(), "\n");
}
static auto get_subtokens(
    const std::vector<Token>& input_tokens,
    std::remove_reference<decltype(input_tokens)>::type::size_type i) -> std::
    tuple<decltype(i), std::vector<Token>, unsigned, unsigned, unsigned> {
    std::string paren_type         = input_tokens.at(i);
    std::string closing_paren_type = ((paren_type == "(") ? ")" : "]");
    ++i;

    auto subtokens   = std::vector<Token>{};
    const auto limit = input_tokens.size();

    unsigned balance                         = 1;
    unsigned toplevel_subexpressions_balance = 0;
    unsigned toplevel_subexpressions         = 0;
    while (i < limit) {
        if (input_tokens.at(i) == paren_type) {
            ++balance;
        } else if (input_tokens.at(i) == closing_paren_type) {
            --balance;
        }
        if (input_tokens.at(i) == "(") {
            if ((++toplevel_subexpressions_balance) == 1) {
                ++toplevel_subexpressions;
            }
        }
        if (input_tokens.at(i) == ")") {
            --toplevel_subexpressions_balance;
        }
        if (balance == 0) {
            ++i;
            break;
        }
        subtokens.push_back(input_tokens.at(i));
        ++i;
    }

    return std::
        tuple<decltype(i), std::vector<Token>, unsigned, unsigned, unsigned>{
            i,
            std::move(subtokens),
            balance,
            toplevel_subexpressions_balance,
            toplevel_subexpressions};
}
static auto get_innermost_target_token(const std::vector<Token>& subtokens,
                                       Token const& t) -> Token {
    Token inner_target_token;
    try {
        unsigned long check = 1;
        do {
            /*
             * Loop until first operand's token is not "(".
             * Unwrapping works on second token that appears after the wrap, but
             * if it is also a "(" then lexer must switch to next second token.
             * Example:
             *
             * iinc (iinc (iinc (iinc (arg 0 1))))
             *      |     ^     ^     ^    ^
             *      |     1     3     5    7
             *      |
             *       \
             *       start unwrapping;
             *       1, 3, and 5 must be skipped because they open inner wraps
             * that must be unwrapped before their targets become available
             *
             *       skipping by two allows us to fetch the target value without
             * waiting for instructions to become unwrapped
             */
            inner_target_token = subtokens.at(check);
            check += 2;
        } while (inner_target_token.str() == "(");
    } catch (std::out_of_range const& e) {
        throw Invalid_syntax(t.line(), t.character(), t.str());
    }
    return inner_target_token;
}
static auto get_counter_token(const std::vector<Token>& subtokens,
                              const unsigned toplevel_subexpressions) -> Token {
    return Token{subtokens.at(0).line(),
                 subtokens.at(0).character(),
                 ('%' + str::stringify(toplevel_subexpressions, false))};
}
auto unwrap_lines(std::vector<Token> input_tokens, bool full)
    -> std::vector<Token> {
    decltype(input_tokens) unwrapped_tokens;
    decltype(input_tokens) tokens;
    decltype(input_tokens) final_tokens;

    decltype(tokens.size()) i = 0, limit = input_tokens.size();
    bool invert = false;

    while (i < limit) {
        Token t = input_tokens.at(i);
        if (t == "^") {
            invert = true;
            ++i;
            continue;
        }
        if (t == "(" or t == "[") {
            auto subtokens                           = std::vector<Token>{};
            unsigned balance                         = 1;
            unsigned toplevel_subexpressions_balance = 0;
            unsigned toplevel_subexpressions         = 0;

            tie(i,
                subtokens,
                balance,
                toplevel_subexpressions_balance,
                toplevel_subexpressions) = get_subtokens(input_tokens, i);

            if (t == "(") {
                if (i >= limit and balance != 0) {
                    throw Invalid_syntax(
                        t, "unbalanced parenthesis in wrapped instruction");
                }
                if (subtokens.size() < 2) {
                    throw Invalid_syntax(t,
                                         "at least two tokens are required "
                                         "in a wrapped instruction");
                }

                Token inner_target_token =
                    get_innermost_target_token(subtokens, t);
                unwrap_subtokens(unwrapped_tokens, subtokens, t);
                push_unwrapped_lines(invert,
                                     inner_target_token,
                                     final_tokens,
                                     unwrapped_tokens,
                                     input_tokens,
                                     i);
                if ((not invert) and full) {
                    if (final_tokens.back().str() == "*") {
                        final_tokens.pop_back();
                        final_tokens.emplace_back(
                            inner_target_token.line(),
                            inner_target_token.character(),
                            ('*' + inner_target_token.str().substr(1)));
                        final_tokens.back().original(inner_target_token.str());
                    } else {
                        final_tokens.push_back(inner_target_token);
                    }
                }
            }
            if (t == "[") {
                Token counter_token =
                    get_counter_token(subtokens, toplevel_subexpressions);

                subtokens = unwrap_lines(subtokens, false);

                unwrap_subtokens(unwrapped_tokens, subtokens, t);
                push_unwrapped_lines(invert,
                                     counter_token,
                                     final_tokens,
                                     unwrapped_tokens,
                                     input_tokens,
                                     i);
                if (not invert) {
                    final_tokens.push_back(counter_token);
                }
            }

            invert = false;
            unwrapped_tokens.clear();
            tokens.clear();

            continue;
        }
        final_tokens.push_back(t);
        ++i;
    }

    return final_tokens;
}

auto replace_iotas(std::vector<Token> input_tokens) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};
    auto iotas  = std::vector<unsigned long>{};

    for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size();
         ++i) {
        auto const& token = input_tokens.at(i);

        if (token == ".iota:") {
            if (iotas.empty()) {
                throw viua::cg::lex::Invalid_syntax(
                    token, "'.iota:' directive used outside of iota scope");
            }
            if (not str::isnum(input_tokens.at(i + 1))) {
                throw viua::cg::lex::Invalid_syntax(
                    input_tokens.at(i + 1),
                    ("invalid argument to '.iota:' directive: "
                     + str::strencode(input_tokens.at(i + 1))));
            }

            iotas.back() = stoul(input_tokens.at(i + 1));

            ++i;
            continue;
        }

        if (token == ".function:" or token == ".closure:"
            or token == ".block:") {
            iotas.push_back(1);
        }
        if (token == "[") {
            iotas.push_back(0);
        }
        if ((token == ".end" or token == "]") and not iotas.empty()) {
            iotas.pop_back();
        }

        if (token == "iota") {
            tokens.emplace_back(token.line(),
                                token.character(),
                                str::stringify(iotas.back()++, false));
            tokens.back().original("iota");
        } else if ((token.str().at(0) == '%' or token.str().at(0) == '@'
                    or token.str().at(0) == '*')
                   and token.str().substr(1) == "iota") {
            tokens.emplace_back(
                token.line(),
                token.character(),
                (token.str().at(0) + str::stringify(iotas.back()++, false)));
            tokens.back().original(token);
        } else {
            tokens.push_back(token);
        }
    }

    return tokens;
}

auto replace_defaults(std::vector<Token> input_tokens) -> std::vector<Token> {
    auto tokens = std::vector<Token>{};

    for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size();
         ++i) {
        auto const& token = input_tokens.at(i);
        tokens.push_back(token);

        if (match(input_tokens, i, {"arg", "default"})
            or match(input_tokens, i, {"call", "default"})
            or match(input_tokens, i, {"process", "default"})) {
            ++i;
            tokens.emplace_back(input_tokens.at(i).line(),
                                input_tokens.at(i).character(),
                                "void");
            tokens.back().original("default");
            continue;
        }
        if (match(input_tokens, i, {"integer", "", "default"})) {
            tokens.push_back(input_tokens.at(++i));  // push target register
                                                     // token
            ++i;
            tokens.emplace_back(
                input_tokens.at(i).line(), input_tokens.at(i).character(), "0");
            tokens.back().original("default");
            continue;
        }
        if (match(input_tokens, i, {"float", "", "default"})) {
            tokens.push_back(input_tokens.at(++i));  // push target register
                                                     // token
            ++i;
            tokens.emplace_back(input_tokens.at(i).line(),
                                input_tokens.at(i).character(),
                                "0.0");
            tokens.back().original("default");
            continue;
        }
        if (match(input_tokens, i, {"string", "", "default"})) {
            tokens.push_back(input_tokens.at(++i));  // push target register
                                                     // token
            ++i;
            tokens.emplace_back(input_tokens.at(i).line(),
                                input_tokens.at(i).character(),
                                "\"\"");
            tokens.back().original("default");
            continue;
        }
        if (match(input_tokens, i, {"receive", "", "default"})) {
            ++i;
            if (input_tokens.at(i) == "default") {
                tokens.emplace_back(input_tokens.at(i).line(),
                                    input_tokens.at(i).character(),
                                    "void");
                tokens.back().original("default");
            } else {
                tokens.push_back(input_tokens.at(i));  // push target register
                                                       // token
            }
            ++i;
            tokens.emplace_back(input_tokens.at(i).line(),
                                input_tokens.at(i).character(),
                                "infinity");
            tokens.back().original("default");
            continue;
        }
        if (match(input_tokens, i, {"join", "", "", "default"})) {
            ++i;
            if (input_tokens.at(i) == "default") {
                tokens.emplace_back(input_tokens.at(i).line(),
                                    input_tokens.at(i).character(),
                                    "void");
                tokens.back().original("default");
            } else {
                tokens.push_back(input_tokens.at(i));  // push target register
                                                       // token
            }
            tokens.push_back(input_tokens.at(++i));  // push source register
                                                     // token
            ++i;
            tokens.emplace_back(input_tokens.at(i).line(),
                                input_tokens.at(i).character(),
                                "infinity");
            tokens.back().original("default");
            continue;
        }
    }

    return tokens;
}

auto replace_named_registers(std::vector<Token> input_tokens)
    -> std::vector<Token> {
    auto tokens = std::vector<Token>{};
    map<std::string, std::string> names;
    unsigned open_blocks = 0;

    for (decltype(input_tokens)::size_type i = 0; i < input_tokens.size();
         ++i) {
        auto const& token = input_tokens.at(i);

        if (str::isnum(token)) {
            tokens.push_back(token);
            continue;
        }

        if (token == ".function:" or token == ".closure:"
            or token == ".block:") {
            ++open_blocks;
        }
        if (token == ".end") {
            --open_blocks;
        }

        if (token == ".name:") {
            Token name        = input_tokens.at(i + 2);
            std::string index = input_tokens.at(i + 1).str();

            assert_is_not_reserved_keyword(name, "register name");

            if ((index.at(0) == '%' or index.at(0) == '@' or index.at(0) == '*')
                and str::isnum(index.substr(1), false)) {
                index = index.substr(1);
            }

            if (not str::isnum(index)) {
                throw viua::cg::lex::Invalid_syntax(
                    input_tokens.at(i + 1),
                    ("invalid register index: " + str::strencode(name)
                     + " := " + str::enquote(str::strencode(index))));
            }

            names[name] = index;

            tokens.push_back(token);
            tokens.push_back(input_tokens.at(++i));
            tokens.push_back(input_tokens.at(++i));

            continue;
        }

        if (names.count(token)) {
            tokens.emplace_back(
                token.line(), token.character(), names.at(token));
            tokens.back().original(token.str());
        } else if (token.str().at(0) == '@'
                   and names.count(token.str().substr(1))) {
            tokens.emplace_back(token.line(),
                                token.character(),
                                ("@" + names.at(token.str().substr(1))));
            tokens.back().original(token.str());
        } else if (token.str().at(0) == '*'
                   and names.count(token.str().substr(1))) {
            tokens.emplace_back(token.line(),
                                token.character(),
                                ("*" + names.at(token.str().substr(1))));
            tokens.back().original(token.str());
        } else if (token.str().at(0) == '%'
                   and names.count(token.str().substr(1))) {
            tokens.emplace_back(token.line(),
                                token.character(),
                                ("%" + names.at(token.str().substr(1))));
            tokens.back().original(token.str());
        } else {
            tokens.push_back(token);
        }

        if (open_blocks == 0) {
            names.clear();
        }
    }

    return tokens;
}
}}}  // namespace viua::cg::lex
