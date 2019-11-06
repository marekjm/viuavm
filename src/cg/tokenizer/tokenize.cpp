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

#include <sstream>
#include <viua/cg/tokenizer.h>
#include <viua/support/string.h>

namespace viua { namespace cg { namespace tokenizer {
auto tokenize(std::string const& s) -> std::vector<std::string>
{
    auto tokens = std::vector<std::string>{};
    auto token  = std::ostringstream{};
    token.str("");
    for (auto i = decltype(s.size()){0}; i < s.size(); ++i) {
        if (s[i] == ' ' and token.str().size()) {
            tokens.emplace_back(token.str());
            token.str("");
            continue;
        }
        if (s[i] == ' ') {
            continue;
        }
        if (s[i] == '^') {
            if (token.str().size()) {
                tokens.emplace_back(token.str());
                token.str("");
            }
            tokens.emplace_back("^");
        }
        if (s[i] == '(' or s[i] == ')') {
            if (token.str().size()) {
                tokens.emplace_back(token.str());
                token.str("");
            }
            tokens.emplace_back((s[i] == '(' ? "(" : ")"));
            continue;
        }
        if (s[i] == '[' or s[i] == ']') {
            if (token.str().size()) {
                tokens.emplace_back(token.str());
                token.str("");
            }
            tokens.emplace_back((s[i] == '[' ? "[" : "]"));
            continue;
        }
        if (s[i] == '{' or s[i] == '}') {
            if (token.str().size()) {
                tokens.emplace_back(token.str());
                token.str("");
            }
            tokens.emplace_back((s[i] == '{' ? "{" : "}"));
            continue;
        }
        if (s[i] == '"' or s[i] == '\'') {
            auto const ss = str::extract(s.substr(i));
            i += (ss.size() - 1);
            tokens.emplace_back(ss);
            continue;
        }
        token << s[i];
    }
    if (token.str().size()) {
        tokens.emplace_back(token.str());
    }
    return tokens;
}
}}}  // namespace viua::cg::tokenizer
