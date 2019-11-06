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

#ifndef VIUA_UTIL_VECTOR_VIEW_H
#define VIUA_UTIL_VECTOR_VIEW_H

#include <stdexcept>
#include <string>
#include <vector>

namespace viua { namespace util {
template<typename T> class vector_view {
  public:
    using container_type  = std::vector<T>;
    using size_type       = typename container_type::size_type;
    using difference_type = typename container_type::difference_type;

  private:
    container_type const& vec;
    size_type const offset;

    auto check_offset() const -> void
    {
        if (offset > vec.size()) {
            throw std::out_of_range{"offset out of range: "
                                    + std::to_string(offset) + " > "
                                    + std::to_string(vec.size())};
        }
    }

  public:
    auto at(size_type const i) const -> T const& { return vec.at(offset + i); }
    auto size() const -> size_type { return (vec.size() - offset); }

    auto advance(size_type const n) const -> vector_view<T>
    {
        return vector_view<T>{vec, offset + n};
    }

    auto begin() const -> decltype(vec.begin())
    {
        return vec.begin() + static_cast<difference_type>(offset);
    }
    auto end() const -> decltype(vec.end()) { return vec.end(); }

    vector_view(container_type const& v, size_type const o) : vec{v}, offset{o}
    {
        check_offset();
    }
    vector_view(vector_view<T> const& v, size_type const o)
            : vec{v.vec}, offset{v.offset + o}
    {
        check_offset();
    }
};
}}  // namespace viua::util

#endif
