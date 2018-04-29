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
#include <viua/front/asm.h>
#include <viua/support/string.h>
using namespace std;


auto decode_line_tokens(vector<std::string> const& tokens)
    -> vector<vector<std::string>> {
    auto decoded_lines = vector<vector<std::string>>{};
    auto main_line     = vector<std::string>{};

    auto i      = std::remove_reference_t<decltype(tokens)>::size_type{0};
    auto invert = false;
    while (i < tokens.size()) {
        if (tokens.at(i) == "^") {
            invert = true;
            ++i;
            continue;
        }
        if (tokens.at(i) == "(") {
            vector<std::string> subtokens;
            ++i;
            auto balance = unsigned{1};
            while (i < tokens.size()) {
                if (tokens.at(i) == "(") {
                    ++balance;
                }
                if (tokens.at(i) == ")") {
                    --balance;
                }
                if (balance == 0) {
                    break;
                }
                subtokens.emplace_back(tokens.at(i));
                ++i;
            }
            auto const sublines = decode_line_tokens(subtokens);
            for (auto const& each : sublines) {
                decoded_lines.emplace_back(each);
            }
            main_line.emplace_back(sublines.at(sublines.size() - 1).at(1));
            ++i;
            continue;
        }
        if (tokens.at(i) == "[") {
            auto subtokens = vector<std::string>{};
            ++i;
            auto balance                  = unsigned{1};
            auto toplevel_subexpr_balance = unsigned{0};
            auto len                      = decltype(i){0};
            while (i < tokens.size()) {
                if (tokens.at(i) == "[") {
                    ++balance;
                }
                if (tokens.at(i) == "]") {
                    --balance;
                }
                if (tokens.at(i) == "(") {
                    if ((++toplevel_subexpr_balance) == 1) {
                        ++len;
                    }
                }
                if (tokens.at(i) == ")") {
                    --toplevel_subexpr_balance;
                }
                if (balance == 0) {
                    break;
                }
                subtokens.emplace_back(tokens.at(i));
                ++i;
            }
            auto sublines = decode_line_tokens(subtokens);
            sublines.pop_back();
            for (auto const& each : sublines) {
                decoded_lines.emplace_back(each);
            }
            main_line.emplace_back(str::stringify(len));
            ++i;
            continue;
        }
        main_line.emplace_back(tokens.at(i));
        ++i;
    }

    if (invert) {
        decoded_lines.insert(decoded_lines.begin(), main_line);
    } else {
        decoded_lines.emplace_back(main_line);
    }

    return decoded_lines;
}
auto decode_line(std::string const& s) -> vector<vector<std::string>> {
    return decode_line_tokens(viua::cg::tokenizer::tokenize(s));
}
