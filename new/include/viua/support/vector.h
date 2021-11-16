/*
 *  Copyright (C) 2021 Marek Marecki
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

#ifndef VIUA_SUPPORT_VECTOR_H
#define VIUA_SUPPORT_VECTOR_H

#include <stddef.h>

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>


namespace viua::support {
template<typename T> struct vector_view {
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = T const*;
    using reference       = T&;
    using const_reference = T const&;
    using const_iterator  = typename std::vector<T>::const_iterator;
    using iterator        = typename std::vector<T>::iterator;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

  private:
    const_pointer base{nullptr};
    const_pointer finish{nullptr};

  public:
    vector_view(std::vector<T> const& v) : base{&v[0]}, finish{base + v.size()}
    {}
    vector_view(vector_view<T> const& v) : base{v.base}, finish{v.finish}
    {}
    vector_view(T const* b, size_t const c) : base{b}, finish{b + c}
    {}

    constexpr auto begin() const noexcept -> const_pointer
    {
        return base;
    }
    constexpr auto end() const noexcept -> const_pointer
    {
        return finish;
    }

    constexpr auto operator[](size_type const i) const noexcept
        -> const_reference
    {
        return *(base + i);
    }
    constexpr auto at(size_type const i) const -> const_reference
    {
        if (i >= size()) {
            throw std::out_of_range{"viua::support::vector_view::at"};
        }
        return *(base + i);
    }
    constexpr auto front() const noexcept -> const_reference
    {
        return *base;
    }
    constexpr auto back() const noexcept -> const_reference
    {
        return *(finish - 1);
    }
    constexpr auto data() const noexcept -> const_pointer
    {
        return base;
    }

    constexpr auto size() const noexcept -> size_type
    {
        return std::distance(base, finish);
    }
    constexpr auto length() const noexcept -> size_type
    {
        return size();
    }
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return (size() == 0);
    }

    constexpr auto remove_prefix(size_type const n) -> void
    {
        if (n > size()) {
            throw std::out_of_range{
                "viua::support::vector_view::remove_prefix"};
        }
        base += n;
    }
    constexpr auto remove_suffix(size_type const n) -> void
    {
        if (n > size()) {
            throw std::out_of_range{
                "viua::support::vector_view::remove_prefix"};
        }
        finish -= n;
    }
};
}  // namespace viua::support

#endif
