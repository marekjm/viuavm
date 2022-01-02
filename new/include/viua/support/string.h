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

#ifndef VIUA_SUPPORT_STRING_H
#define VIUA_SUPPORT_STRING_H

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>


namespace viua::support::string {
constexpr auto CORNER_QUOTE_LL = std::string_view{"⌞"};  // lower left
constexpr auto CORNER_QUOTE_UR = std::string_view{"⌝"};  // upper right

auto quoted(std::string_view const) -> std::string;
auto quote_squares(std::string_view const) -> std::string;

auto unescape(std::string_view const) -> std::string;

using LevenshteinDistance = std::string::size_type;
using DistancePair        = std::pair<LevenshteinDistance, std::string>;
auto levenshtein(std::string const, std::string const) -> LevenshteinDistance;
auto levenshtein_filter(std::string const,
                        std::set<std::string> const&,
                        LevenshteinDistance const = LevenshteinDistance{
                            0xffffffffffffffff}) -> std::set<DistancePair>;
template<typename T>
auto levenshtein_filter(std::string const needle,
                        std::map<std::string, T> const& haystack,
                        LevenshteinDistance const dist = LevenshteinDistance{
                            0xffffffffffffffff}) -> std::set<DistancePair>
{
    auto candidates = std::set<std::string>{};
    for (auto const& each : haystack) {
        candidates.insert(each.first);
    }
    return levenshtein_filter(needle, candidates, dist);
}
auto levenshtein_best(std::string const,
                      std::set<DistancePair> const&,
                      LevenshteinDistance const) -> DistancePair;
}  // namespace viua::support::string

#endif
