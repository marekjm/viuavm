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

#ifndef VIUA_VM_CORE_CELL_H
#define VIUA_VM_CORE_CELL_H

#include <stdint.h>

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>


namespace viua::vm::types {
struct Cell_view {
    using boxed_type = class Value;
    using void_type  = std::monostate;
    using value_type = std::variant<void_type,
                                    std::reference_wrapper<int64_t>,
                                    std::reference_wrapper<uint64_t>,
                                    std::reference_wrapper<float>,
                                    std::reference_wrapper<double>,
                                    std::reference_wrapper<boxed_type>>;

    value_type content;

    Cell_view() = delete;
    Cell_view(class Cell&);
    explicit Cell_view(boxed_type&);

    template<typename T> auto holds() const -> bool
    {
        if constexpr (std::is_same_v<T, void>) {
            return std::holds_alternative<void_type>(content);
        }

        using Tr = std::reference_wrapper<T>;

        if constexpr (std::is_same_v<T, int64_t>) {
            return std::holds_alternative<Tr>(content);
        }
        if constexpr (std::is_same_v<T, uint64_t>) {
            return std::holds_alternative<Tr>(content);
        }
        if constexpr (std::is_same_v<T, float>) {
            return std::holds_alternative<Tr>(content);
        }
        if constexpr (std::is_same_v<T, double>) {
            return std::holds_alternative<Tr>(content);
        }

        /*
         * All subsequent checks do not make sense if the cell does not hold a
         * boxed value. Let's verify this once instead of repeating the check in
         * every following if statement.
         */
        using Br = std::reference_wrapper<boxed_type>;
        if (not std::holds_alternative<Br>(content)) {
            return false;
        }

        /*
         * Let's check for types derived from the root type ie, Value. We could
         * rely on the catch-all dynamic_cast to T, but the explicitness here is
         * good, I think.
         */
        if constexpr (std::is_convertible_v<T*, viua::vm::types::Value*>) {
            return (dynamic_cast<T*>(&std::get<Br>(content).get()) != nullptr);
        }

        /*
         * This last check is necessary because we can see if the cell holds a
         * trait ie, if it holds a value which can be cast to a trait type.
         * These types are standalone, abstract classes so are not readily
         * convertible to the root type ie, Value and are only used to mark
         * types as implementing certain behaviours or functionalities eg, a
         * possibility of comparison with other types, or conversion to string.
         */
        if constexpr (std::is_class_v<T>) {
            return (dynamic_cast<T*>(&std::get<Br>(content).get()) != nullptr);
        }

        return false;
    }

    auto is_void() const -> bool
    {
        return holds<void>();
    }

    template<typename T> auto get() -> T&
    {
        return std::get<std::reference_wrapper<T>>(content);
    }
    template<typename T> auto get() const -> T const&
    {
        return std::get<std::reference_wrapper<T>>(content);
    }

    template<typename T>
    auto boxed_of() -> std::optional<std::reference_wrapper<T>>
    {
        if (holds<T>()) {
            using Rt  = std::reference_wrapper<boxed_type>;
            auto& ref = std::get<Rt>(content).get();
            return dynamic_cast<T&>(ref);
        } else {
            return {};
        }
    }
    template<typename T>
    auto boxed_of() const -> std::optional<std::reference_wrapper<T const>>
    {
        if (holds<T>()) {
            using Rt        = std::reference_wrapper<boxed_type>;
            auto const& ref = std::get<Rt>(content).get();
            return dynamic_cast<T const&>(ref);
        } else {
            return {};
        }
    }
};

struct Cell {
    using boxed_type = std::unique_ptr<class Value>;
    using void_type  = std::monostate;
    using value_type =
        std::variant<void_type, int64_t, uint64_t, float, double, boxed_type>;

    value_type content;

    Cell() = default;
    Cell(Cell&& c) : content{std::move(c.content)}
    {
        c.content = void_type{};
    }
    template<typename T> explicit Cell(T&& v) : content{std::move(v)}
    {}

    auto view() -> Cell_view
    {
        return Cell_view{*this};
    }

    auto operator=(Cell&& v) -> Cell&
    {
        content   = std::move(v.content);
        v.content = void_type{};
        return *this;
    }
    template<typename T> auto operator=(T&& v) -> Cell&
    {
        content = std::move(v);
        return *this;
    }

    template<typename T> auto get() -> T&
    {
        return std::get<T>(content);
    }
    template<typename T> auto get() const -> T const&
    {
        return std::get<T>(content);
    }
};
}  // namespace viua::vm::types

#endif
