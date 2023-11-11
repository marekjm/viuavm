/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <viua/support/string.h>


namespace viua::support::string {
auto quoted(std::string_view const sv) -> std::string
{
    auto out = std::ostringstream{};
    out << std::quoted(sv);

    auto const tmp = out.str();
    out.str("");

    for (auto const each : tmp) {
        if (each == '\n') {
            out << "\\n";
        } else {
            out << each;
        }
    }

    return out.str();
}

auto quote_squares(std::string_view const sv) -> std::string
{
    auto out = std::ostringstream{};
    out << CORNER_QUOTE_LL << sv << CORNER_QUOTE_UR;
    return out.str();
}
auto quote_math_angle(std::string_view const sv) -> std::string
{
    auto out = std::ostringstream{};
    out << MATHEMATICAL_ANGLE_BRACKET_LEFT << sv
        << MATHEMATICAL_ANGLE_BRACKET_RIGHT;
    return out.str();
}
auto quote_fancy(std::string_view const sv) -> std::string
{
    auto out = std::ostringstream{};
    out << SINGLE_QUOTATION_MARK_LEFT << sv << SINGLE_QUOTATION_MARK_RIGHT;
    return out.str();
}

auto unescape(std::string_view const sv) -> std::string
{
    /*
     * Decode escape sequences in strings.
     *
     * This function recognizes escape sequences as listed on:
     * http://en.cppreference.com/w/cpp/language/escape
     * The function does not recognize sequences for:
     *     - arbitrary octal numbers (escape: \nnn),
     *     - arbitrary hexadecimal numbers (escape: \xnn),
     *     - short arbitrary Unicode values (escape: \unnnn),
     *     - long arbitrary Unicode values (escape: \Unnnnnnnn),
     *
     * If a character that does not encode an escape sequence is
     * preceded by a backslash (\\) the function consumes the backslash and
     * leaves only the "escaped" character in the output std::string.
     */
    auto decoded = std::ostringstream{};
    auto c       = char{};
    for (auto i = std::string::size_type{0}; i < sv.size(); ++i) {
        c = sv[i];
        if (c == '\\' and i < (sv.size() - 1)) {
            ++i;
            switch (sv[i]) {
            case '\'':
                c = '\'';
                break;
            case '"':
                c = '"';
                break;
            case '?':
                c = '?';
                break;
            case '\\':
                c = '\\';
                break;
            case 'a':
                c = '\a';
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'v':
                c = '\v';
                break;
            default:
                c = sv[i];
            }
        }
        decoded << c;
    }
    return decoded.str();
}

auto levenshtein(std::string_view const source, std::string_view const target)
    -> LevenshteinDistance
{
    if (not source.size()) {
        return target.size();
    }
    if (not target.size()) {
        return source.size();
    }

    auto distance_matrix = std::vector<std::vector<LevenshteinDistance>>{};

    distance_matrix.reserve(source.size());
    for (auto i = LevenshteinDistance{0}; i < source.size() + 1; ++i) {
        auto row = decltype(distance_matrix)::value_type{};
        row.reserve(target.size());
        for (auto j = LevenshteinDistance{0}; j < target.size() + 1; ++j) {
            row.push_back(0);
        }
        distance_matrix.push_back(std::move(row));
    }
    for (auto i = LevenshteinDistance{0}; i < source.size() + 1; ++i) {
        distance_matrix.at(i).at(0) = i;
    }
    for (auto i = LevenshteinDistance{0}; i < target.size() + 1; ++i) {
        distance_matrix.at(0).at(i) = i;
    }

    for (auto i = LevenshteinDistance{1}; i < source.size() + 1; ++i) {
        for (auto j = LevenshteinDistance{1}; j < target.size() + 1; ++j) {
            auto cost = LevenshteinDistance{0};

            cost = (source.at(i - 1) != target.at(j - 1));

            auto deletion     = distance_matrix.at(i - 1).at(j) + 1;
            auto insertion    = distance_matrix.at(i).at(j - 1) + 1;
            auto substitution = distance_matrix.at(i - 1).at(j - 1) + cost;

            distance_matrix.at(i).at(j) =
                std::min(std::min(deletion, insertion), substitution);
        }
    }

    return distance_matrix.at(source.size() - 1).at(target.size() - 1);
}
auto levenshtein_filter(std::string_view const source,
                        std::set<std::string_view> const& candidates,
                        LevenshteinDistance const limit)
    -> std::set<DistancePair>
{
    auto matched = std::set<DistancePair>{};

    for (auto const& each : candidates) {
        if (auto distance = levenshtein(source, each); distance <= limit) {
            matched.emplace(distance, each);
        }
    }

    return matched;
}
auto levenshtein_best(std::string_view const source,
                      std::set<DistancePair> const& candidates,
                      LevenshteinDistance const limit) -> DistancePair
{
    auto best = DistancePair{0xffffffffffffffff, source};

    for (auto const& each : candidates) {
        if (each.first <= limit and each.first < best.first) {
            best = each;
            continue;
        }
    }

    return best;
}
}  // namespace viua::support::string
