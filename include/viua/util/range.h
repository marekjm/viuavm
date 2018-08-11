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

#ifndef VIUA_UTIL_RANGE_H
#define VIUA_UTIL_RANGE_H

#include <numeric>
#include <vector>

namespace viua { namespace util {
template<typename T> class Range {
    std::vector<T> v;
  public:
    Range(T const upper) {
        v.reserve(upper);
        v.resize(upper);
        std::iota(v.begin(), v.end(), T{0});
    }

    auto begin() const -> decltype(v.begin()) {
        return v.begin();
    }
    auto end() const -> decltype(v.end()) {
        return v.end();
    }
};
}}  // namespace viua::util

#endif
