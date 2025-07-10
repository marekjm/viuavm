/*
 *  Copyright (C) 2023, 2025 Marek Marecki
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

#ifndef VIUA_SUPPORT_MEMORY_H
#define VIUA_SUPPORT_MEMORY_H

#include <stdint.h>
#include <string.h>

namespace viua::support {
template<typename T>
auto memload(
    void const* const src) -> T
{
    auto tmp = T{};
    memcpy(&tmp, src, sizeof(T));
    return tmp;
}
}  // namespace viua::support

namespace viua {
template<typename T>
struct view_ptr {
    using element_type = T;
    using pointer      = element_type*;

    pointer viewed_ptr{ nullptr };

    explicit view_ptr(
        pointer const ptr = nullptr)
        : viewed_ptr{ ptr }
    {}
    view_ptr(
        std::nullptr_t)
        : viewed_ptr{ nullptr }
    {}

    constexpr auto reset(
        pointer ptr = pointer{}) noexcept -> void
    {
        viewed_ptr = ptr;
    }

    constexpr auto empty() const noexcept -> bool
    {
        return (viewed_ptr == nullptr);
    }
    constexpr operator bool() const noexcept
    {
        return not empty();
    }

    auto get() const -> pointer
    {
        if (empty()) {
            throw std::runtime_error{ "view_ptr::get: nullptr" };
        }
        return viewed_ptr;
    }

    auto operator*() const -> std::add_lvalue_reference<element_type>::type
    {
        return *get();
    }
    auto operator->() const -> pointer
    {
        return get();
    }
};
}  // namespace viua

#endif
