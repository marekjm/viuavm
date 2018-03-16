/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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

#ifndef SUPPORT_STRING_H
#define SUPPORT_STRING_H

#pragma once

#include <sstream>
#include <string>
#include <vector>


namespace str {
    auto startswith(std::string const& s, std::string const& w) -> bool;
    auto startswithchunk(std::string const& s, std::string const& w) -> bool;
    auto endswith(std::string const& s, std::string const& w) -> bool;

    auto isnum(std::string const& s, bool const negatives = true) -> bool;
    auto ishex(std::string const& s, bool const negatives = true) -> bool;
    auto isfloat(std::string const& s, bool const negatives = true) -> bool;
    auto isid(std::string const& s) -> bool;

    auto is_binary_literal(std::string const) -> bool;
    auto is_boolean_literal(std::string const) -> bool;
    auto is_void(std::string const) -> bool;
    auto is_atom_literal(std::string const) -> bool;
    auto is_text_literal(std::string const) -> bool;
    auto is_timeout_literal(std::string const s) -> bool;
    auto is_register_set_name(std::string const) -> bool;

    auto sub(std::string const& s, std::string::size_type b = 0, ssize_t const e = -1) -> std::string;

    auto extract(std::string const& s) -> std::string;
    auto chunk(std::string const& s, bool const ignore_leading_ws = true) -> std::string;
    auto chunks(std::string const& s) -> std::vector<std::string>;

    auto join(std::string const& s, std::vector<std::string> const& v) -> std::string;
    template<typename T> auto join(std::vector<std::string> const& seq, T const& delim) -> std::string {
        auto const sz = seq.size();
        std::ostringstream oss;
        for (std::remove_const_t<decltype(sz)> i = 0; i < sz; ++i) {
            oss << seq[i];
            if (i < (sz - 1)) {
                oss << delim;
            }
        }
        return oss.str();
    }

    template<typename T> auto strmul(T const& s, size_t const times) -> std::string {
        std::ostringstream oss;
        for (std::remove_const_t<decltype(times)> i = 0; i < times; ++i) {
            oss << s;
        }
        return oss.str();
    }

    auto lstrip(std::string const& s) -> std::string;

    std::string::size_type lshare(std::string const& s, std::string const& w);
    auto contains(std::string const& s, char const c) -> bool;

    using LevenshteinDistance = std::string::size_type;
    using DistancePair = std::pair<LevenshteinDistance, std::string>;
    auto levenshtein(std::string const, std::string const) -> LevenshteinDistance;
    auto levenshtein_filter(std::string const, std::vector<std::string> const&, LevenshteinDistance const)
        -> std::vector<DistancePair>;
    auto levenshtein_best(std::string const, std::vector<std::string> const&, LevenshteinDistance const)
        -> DistancePair;

    auto enquote(std::string const&, char const = '"') -> std::string;
    auto strdecode(std::string const&) -> std::string;
    auto strencode(std::string const&) -> std::string;

    auto stringify(std::vector<std::string> const&) -> std::string;
    template<class T> auto stringify(T const o, bool const nl = true) -> std::string {
        std::ostringstream oss;
        oss << o;
        if (nl) {
            oss << "\n";
        }
        return oss.str();
    }
}  // namespace str


#endif
