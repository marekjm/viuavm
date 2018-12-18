/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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
#include <viua/assembler/frontend/parser.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
#include <viua/support/string.h>
using namespace std;


namespace viua { namespace assembler { namespace frontend {
using viua::front::assembler::Invocables;

auto gather_functions(std::vector<viua::cg::lex::Token> const& tokens)
    -> Invocables {
    auto invocables = Invocables{};

    invocables.names      = ::assembler::ce::get_function_names(tokens);
    invocables.signatures = ::assembler::ce::get_signatures(tokens);
    invocables.tokens =
        ::assembler::ce::get_invokables_token_bodies("function", tokens);
    for (auto const& each :
         ::assembler::ce::get_invokables_token_bodies("closure", tokens)) {
        invocables.tokens[each.first] = each.second;
    }

    return invocables;
}

auto gather_blocks(std::vector<viua::cg::lex::Token> const& tokens)
    -> Invocables {
    auto invocables = Invocables{};

    invocables.names      = ::assembler::ce::get_block_names(tokens);
    invocables.signatures = ::assembler::ce::get_block_signatures(tokens);
    invocables.tokens =
        ::assembler::ce::get_invokables_token_bodies("block", tokens);

    return invocables;
}

auto gather_meta_information(std::vector<viua::cg::lex::Token> const& tokens)
    -> std::map<std::string, std::string> {
    auto meta_information = map<std::string, std::string>{};

    for (auto i = std::remove_reference<decltype(tokens)>::type::size_type{0};
         i < tokens.size();
         ++i) {
        if (tokens.at(i) == ".info:") {
            auto const key   = tokens.at(i + 1);
            auto const value = tokens.at(i + 2);
            if (key == "\n") {
                throw viua::cg::lex::Invalid_syntax(
                    tokens.at(i), "missing key and value in .info: directive");
            }
            if (value == "\n") {
                throw viua::cg::lex::Invalid_syntax(
                    tokens.at(i), "missing value in .info: directive");
            }
            meta_information.emplace(
                key,
                str::strdecode(value.str().substr(1, value.str().size() - 2)));
        }
    }

    return meta_information;
}
}}}  // namespace viua::assembler::frontend
