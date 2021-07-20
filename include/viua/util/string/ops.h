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

#ifndef VIUA_UTIL_STRING_OPS_H
#define VIUA_UTIL_STRING_OPS_H

#include <string>
#include <vector>

namespace viua { namespace util { namespace string { namespace ops {
auto extract(std::string const&) -> std::string;
auto strencode(std::string const&) -> std::string;
auto strdecode(std::string const) -> std::string;
auto quoted(std::string const&) -> std::string;

using LevenshteinDistance = std::string::size_type;
using DistancePair        = std::pair<LevenshteinDistance, std::string>;
auto levenshtein(std::string const, std::string const) -> LevenshteinDistance;
auto levenshtein_filter(std::string const,
                        std::vector<std::string> const&,
                        LevenshteinDistance const) -> std::vector<DistancePair>;
auto levenshtein_best(std::string const,
                      std::vector<std::string> const&,
                      LevenshteinDistance const) -> DistancePair;
}}}}  // namespace viua::util::string::ops

#endif
