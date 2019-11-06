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

namespace viua { namespace cg { namespace lex {
static auto check_for_missing_colons(std::vector<Token> const& tokens) -> void
{
    using size_type = std::remove_reference_t<decltype(tokens)>::size_type;
    for (auto i = size_type{0}; i < tokens.size(); ++i) {
        if (tokens.at(i) != ".") {
            continue;
        }

        auto const need_colon = std::set<std::string>{"function",
                                                      "closure",
                                                      "block",
                                                      "signature",
                                                      "bsignature",
                                                      "info",
                                                      "name",
                                                      "import",
                                                      "mark",
                                                      "iota"};
        if (need_colon.count(tokens.at(i + 1)) == 0) {
            continue;
        }

        if (tokens.at(i + 2) != ":") {
            throw viua::cg::lex::Invalid_syntax(
                tokens.at(i + 1),
                ("missing ':' after `" + tokens.at(i + 1).str() + "'"))
                .add(tokens.at(i));
        }
    }
}

auto cook(std::vector<Token> tokens, bool const with_replaced_names)
    -> std::vector<Token>
{
    /*
     * Remove whitespace as first step to reduce noise in token stream.
     * Remember not to remove newlines ('\n') because they act as separators
     * in Viua assembly language, and are thus quite important.
     */
    tokens = remove_spaces(tokens);
    tokens = remove_comments(tokens);

    /*
     * Reduce consecutive newline tokens to a single newline.
     * For example, ["foo", "\n", "\n", "\n", "bar"] is reduced to ["foo", "\n",
     * "bar"]. This further reduces noise, and allows later reductions to make
     * the assumption that there is always only one newline when newline may
     * appear.
     */
    tokens = reduce_newlines(tokens);

    /*
     * Reduce directives as lexer emits them as multiple tokens.
     * This lets later reductions to check jsut one token to see if it is an
     * assembler directive instead of looking two or three tokens ahead.
     */
    tokens = reduce_function_directive(tokens);
    tokens = reduce_closure_directive(tokens);
    tokens = reduce_end_directive(tokens);
    tokens = reduce_signature_directive(tokens);
    tokens = reduce_bsignature_directive(tokens);
    tokens = reduce_block_directive(tokens);
    tokens = reduce_info_directive(tokens);
    tokens = reduce_name_directive(tokens);
    tokens = reduce_import_directive(tokens);
    tokens = reduce_mark_directive(tokens);
    tokens = reduce_iota_directive(tokens);

    check_for_missing_colons(tokens);

    /*
     * Reduce directive-looking strings.
     */
    tokens = reduce_token_sequence(tokens, {".", "", ":"});

    /*
     * Reduce double-colon token to make life easier for name reductions.
     */
    tokens = reduce_double_colon(tokens);

    tokens = reduce_left_attribute_bracket(tokens);
    tokens = reduce_right_attribute_bracket(tokens);

    /*
     * Then, reduce function signatues and names.
     * Reduce function names first because they have the arity suffix
     * ('/<integer>') apart from the 'name ("::" name)*' core which both
     * reducers recognise.
     */
    tokens = reduce_function_signatures(tokens);
    tokens = reduce_names(tokens);

    /*
     * Reduce other tokens that are not lexed as single entities, e.g.
     * floating-point literals, or
     * @-prefixed registers.
     *
     * The order should not be changed as the functions make assumptions about
     * input list, and parsing may break if the assumptions are false. Order may
     * be changed if the externally visible outputs from assembler (i.e.
     * compiled bytecode) do not change.
     */
    tokens = reduce_offset_jumps(tokens);
    tokens = reduce_at_prefixed_registers(tokens);
    tokens = reduce_floats(tokens);

    /*
     * Replace 'iota' keywords with their integers.
     * This **MUST** be run before unwrapping instructions because '[]' are
     * needed to correctly replace iotas inside them (the '[]' create new iota
     * scopes).
     */
    tokens = replace_iotas(tokens);

    /*
     * Replace 'default' keywords with their values.
     * This **MUST** be run before unwrapping instructions because unwrapping
     * copies and rearranges tokens in a list so "default" may be copied
     * somewhere where the expansion would be incorrect, or illegal.
     */
    tokens = replace_defaults(tokens);

    /*
     * Unroll instruction wrapped in '()' and '[]'.
     * This makes assembler's and static analyser's work easier since they can
     * deal with linear token sequence.
     */
    tokens = unwrap_lines(tokens);

    /*
     * Reduce @- and *-prefixed registers once more.
     * This is to support constructions like this:
     *
     *  print *(ptr X A)
     *
     * where the prefix and register name are disconnected before the lines are
     * unwrapped.
     */
    tokens = reduce_at_prefixed_registers(tokens);

    /*
     * Replace register names set by '.name:' directive by their register
     * indexes. At later processing stages functions need not concern themselves
     * with names and may operate on register indexes only.
     */
    if (with_replaced_names) {
        tokens = replace_named_registers(tokens);
    }

    /*
     * Move inlined blocks out of their functions.
     * This makes life easier for functions at later processing stages as they
     * do not have to deal with nested blocks.
     *
     * Still, this must be run after iota expansion because nested blocks should
     * share iotas with their functions.
     */
    tokens = move_inline_blocks_out(tokens);

    /*
     * Reduce newlines once more, since unwrap_lines() may sometimes insert a
     * spurious newline into the token stream. It's easier to just clean the
     * newlines up after unwrap_lines() than to add extra ifs to it, and it also
     * helps readability (unwrap_lines() may be less convulted).
     */
    tokens = reduce_newlines(tokens);

    return tokens;
}
}}}  // namespace viua::cg::lex
