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

#include <iomanip>
#include <sstream>
#include <string>
#include <viua/util/string/ops.h>

namespace viua {
namespace util {
namespace string {
namespace ops {
auto extract(std::string const& s) -> std::string {
    /** Extracts *enquoted chunk*.
     *
     *  It is particularly useful if you have a std::string encoded in another
     * std::string.
     *
     *  This function will return `"Hello 'Beautiful' World!"` when fed `"Hello
     * 'Beautiful' World!" some other (42) things;`, and will return `'Hello
     * "Beautiful" World!'` when fed `'Hello "Beautiful" World!' some other (42)
     * things;`.
     *  Starting quote character is irrelevant.
     *
     *  In fact, this function will treat *the first character* of the
     * std::string it is fed as a delimiter for std::string extraction -
     * whatever that may be (e.g. the backtick character) so you can get
     * creative. One character that is not recommended for use as a delimiter is
     * the backslash as it is treated specially (as the escape character) by
     * this function.
     */
    if (s.size() == 0) {
        return std::string("");
    }

    auto chnk  = std::ostringstream{};
    auto quote = char{};
    chnk << (quote = s[0]);

    auto backs = std::string::size_type{0};
    for (auto i = std::string::size_type{1}; i < s.size(); ++i) {
        chnk << s[i];
        if (backs and s[i] != '\\' and s[i] != quote) {
            backs = 0;
            continue;
        }
        if (s[i] == quote and ((backs % 2) != 0)) {
            backs = 0;
            continue;
        } else if (s[i] == quote and ((backs % 2) == 0)) {
            break;
        }
        if (s[i] == '\\') {
            ++backs;
        }
    }

    return chnk.str();
}

auto strencode(std::string const& s) -> std::string {
    /** Encode escape sequences in strings.
     *
     *  This function recognizes escape sequences as listed on:
     *  http://en.cppreference.com/w/cpp/language/escape
     *  The function does not recognize sequences for:
     *      - arbitrary octal numbers (escape: \nnn),
     *      - arbitrary hexadecimal numbers (escape: \xnn),
     *      - short arbitrary Unicode values (escape: \unnnn),
     *      - long arbitrary Unicode values (escape: \Unnnnnnnn),
     *
     */
    auto encoded = std::ostringstream{};
    auto c       = char{};
    auto escape  = bool{false};
    for (auto i = std::string::size_type{0}; i < s.size(); ++i) {
        switch (s[i]) {
        case '\\':
            escape = true;
            c      = '\\';
            break;
        case '\a':
            escape = true;
            c      = 'a';
            break;
        case '\b':
            escape = true;
            c      = 'b';
            break;
        case '\f':
            escape = true;
            c      = 'f';
            break;
        case '\n':
            escape = true;
            c      = 'n';
            break;
        case '\r':
            escape = true;
            c      = 'r';
            break;
        case '\t':
            escape = true;
            c      = 't';
            break;
        case '\v':
            escape = true;
            c      = 'v';
            break;
        default:
            escape = false;
            c      = s[i];
        }
        if (escape) {
            encoded << '\\';
        }
        encoded << c;
    }
    return encoded.str();
}

auto quoted(std::string const& s) -> std::string {
    auto o = std::ostringstream{};
    o << std::quoted(s);
    return o.str();
}

auto levenshtein(std::string const source, std::string const target)
    -> LevenshteinDistance {
    if (not source.size()) {
        return target.size();
    }
    if (not target.size()) {
        return source.size();
    }

    auto distance_matrix = std::vector<std::vector<LevenshteinDistance>>{};

    distance_matrix.reserve(source.size());
    for (auto i = LevenshteinDistance{0}; i < source.size() + 1; ++i) {
        decltype(distance_matrix)::value_type row;
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
auto levenshtein_filter(std::string const source,
                        std::vector<std::string> const& candidates,
                        LevenshteinDistance const limit)
    -> std::vector<DistancePair> {
    auto matched = std::vector<DistancePair>{};

    for (auto const& each : candidates) {
        if (auto distance = levenshtein(source, each); distance <= limit) {
            matched.emplace_back(distance, each);
        }
    }

    return matched;
}
auto levenshtein_best(std::string const source,
                      std::vector<std::string> const& candidates,
                      LevenshteinDistance const limit) -> DistancePair {
    /*
     * Use the maximum unsigned value as the "not found" marker.
     * This is what std::string::npos does.
     */
    auto best = DistancePair{-1, source};

    for (auto const& each : levenshtein_filter(source, candidates, limit)) {
        if ((not best.first) or each.first < best.first) {
            best = each;
            continue;
        }
    }

    return best;
}
}}}}
