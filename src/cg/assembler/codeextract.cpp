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

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>
#include <viua/support/string.h>
using namespace std;


using Token = viua::cg::lex::Token;


auto assembler::ce::getmarks(vector<viua::cg::lex::Token> const& tokens)
    -> map<std::string,
           std::remove_reference<decltype(tokens)>::type::size_type> {
    /** This function will pass over all instructions and
     * gather "marks", i.e. `.mark: <name>` directives which may be used by
     * `jump` and `branch` instructions.
     */
    auto instruction = std::remove_reference<decltype(tokens)>::type::size_type{
        0};  // we need separate instruction counter
             // because number of lines is not
             // exactly number of instructions
    auto marks = map<std::string, decltype(instruction)>{};

    for (auto i = decltype(tokens.size()){0}; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".name:" or tokens.at(i) == ".import:") {
            do {
                ++i;
            } while (i < tokens.size() and tokens.at(i) != "\n");
            continue;
        }
        if (tokens.at(i) == ".mark:") {
            ++i;  // skip ".mark:" token
            assert_is_not_reserved_keyword(tokens.at(i), "marker name");
            marks.emplace(tokens.at(i), instruction);
            ++i;  // skip marker name
            continue;
        }
        if (tokens.at(i) == "\n") {
            ++instruction;
        }
    }

    return marks;
}

auto assembler::ce::getlinks(vector<viua::cg::lex::Token> const& tokens)
    -> vector<std::string> {
    /** This function will pass over all instructions and
     * gather .import: assembler instructions.
     */
    auto links = vector<std::string>{};
    for (auto i = decltype(tokens.size()){0}; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".import:") {
            ++i;  // skip '.import:' token
            if (tokens.at(i) == "\n") {
                throw viua::cg::lex::InvalidSyntax(
                    tokens.at(i), "missing module name in import directive");
            }
            links.emplace_back(tokens.at(i));
            ++i;  // skip module name token
        }
    }
    return links;
}

static auto looks_like_name_definition(Token const t) -> bool {
    return (t == ".function:" or t == ".closure:" or t == ".block:"
            or t == ".signature:" or t == ".bsignature:");
}
static auto get_instruction_block_names(vector<Token> const& tokens,
                                        std::string const directive,
                                        void predicate(Token) = [](Token) {})
    -> vector<std::string> {
    auto names         = vector<std::string>{};
    auto all_names     = vector<std::string>{};
    auto defined_where = map<std::string, Token>{};

    auto const limit       = tokens.size();
    auto const looking_for = std::string{"." + directive + ":"};
    for (auto i = decltype(tokens.size()){0}; i < limit; ++i) {
        if (looks_like_name_definition(tokens.at(i))) {
            auto const first_token_of_block_header_at = i;
            ++i;
            if (i >= limit) {
                throw tokens[i - 1];
            }

            if (tokens.at(i) == "[[") {
                do {
                    ++i;
                } while (tokens.at(i) != "]]");
                ++i;
            }

            if (defined_where.count(tokens.at(i)) > 0) {
                throw viua::cg::lex::TracedSyntaxError()
                    .append(viua::cg::lex::InvalidSyntax(
                        tokens.at(i),
                        ("duplicated name: " + tokens.at(i).str())))
                    .append(viua::cg::lex::InvalidSyntax(
                        defined_where.at(tokens.at(i)),
                        "already defined here:"));
            }

            if (tokens.at(first_token_of_block_header_at) != looking_for) {
                continue;
            }

            predicate(tokens.at(i));
            names.emplace_back(tokens.at(i).str());
            defined_where.emplace(tokens.at(i), tokens.at(i));
        }
    }

    return names;
}
auto assembler::ce::get_function_names(vector<Token> const& tokens)
    -> vector<std::string> {
    auto names = get_instruction_block_names(tokens, "function", [](Token t) {
        assert_is_not_reserved_keyword(t, "function name");
    });
    for (const auto& each : get_instruction_block_names(tokens, "closure")) {
        names.push_back(each);
    }
    return names;
}
auto assembler::ce::get_signatures(vector<Token> const& tokens)
    -> vector<std::string> {
    return get_instruction_block_names(tokens, "signature", [](Token t) {
        assert_is_not_reserved_keyword(t, "function name");
    });
}
auto assembler::ce::get_block_names(vector<Token> const& tokens)
    -> vector<std::string> {
    return get_instruction_block_names(tokens, "block", [](Token t) {
        assert_is_not_reserved_keyword(t, "block name");
    });
}
auto assembler::ce::get_block_signatures(vector<Token> const& tokens)
    -> vector<std::string> {
    return get_instruction_block_names(tokens, "bsignature", [](Token t) {
        assert_is_not_reserved_keyword(t, "block name");
    });
}


static auto get_raw_block_bodies(std::string const& type,
                                 vector<Token> const& tokens)
    -> map<std::string, vector<Token>> {
    auto invokables = map<std::string, vector<Token>>{};

    auto const looking_for = std::string{"." + type + ":"};

    for (auto i = decltype(tokens.size()){0}; i < tokens.size(); ++i) {
        if (tokens[i] == looking_for) {
            ++i;  // skip directive

            if (tokens.at(i) == "[[") {
                while (tokens.at(++i) != "]]") {
                }
                ++i;  // skip closing ]]
            }

            auto const name = tokens[i].str();
            ++i;  // skip name

            auto body = vector<Token>{};
            ++i;  // skip '\n' token
            while (i < tokens.size() and tokens[i].str() != ".end") {
                if (tokens[i] == looking_for) {
                    throw viua::cg::lex::InvalidSyntax(
                        tokens[i],
                        ("another " + type
                         + " opened before assembler reached .end after '"
                         + name + "' " + type));
                }
                body.push_back(tokens[i]);
                ++i;
            }
            ++i;  // skip .end token

            invokables[name] = body;
            body.clear();
        }
    }

    return invokables;
}
auto assembler::ce::get_invokables_token_bodies(const std::string& type,
                                                const vector<Token>& tokens)
    -> map<std::string, vector<Token>> {
    return get_raw_block_bodies(type, tokens);
}
