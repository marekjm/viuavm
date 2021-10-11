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

#ifndef VIUA_VM_TYPES_VALUE_H
#define VIUA_VM_TYPES_VALUE_H

#include <stdint.h>

#include <exception>
#include <functional>
#include <memory>
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
        if (std::is_same_v<
                T,
                void> and std::holds_alternative<void_type>(content)) {
            return true;
        }

        using Tr = std::reference_wrapper<T>;
        return std::holds_alternative<Tr>(content);
    }

    template<typename T> auto get() -> T&
    {
        return std::get<std::reference_wrapper<T>>(content);
    }
    template<typename T> auto get() const -> T const&
    {
        return std::get<std::reference_wrapper<T>>(content);
    }
};

struct Cell {
    using boxed_type = std::unique_ptr<class Value>;
    using void_type  = std::monostate;
    using value_type =
        std::variant<void_type, int64_t, uint64_t, float, double, boxed_type>;

    value_type content;

    Cell() = default;
    template<typename T> explicit Cell(T&& v) : content{std::move(v)}
    {}

    auto view() -> Cell_view
    {
        return Cell_view{*this};
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

class Value {
  public:
    virtual auto type_name() const -> std::string = 0;
    virtual ~Value();

    template<typename Trait>
    auto as_trait(std::function<void(Trait const&)> fn) const -> void
    {
        if (not has_trait<Trait>()) {
            return;
        }

        fn(*dynamic_cast<Trait const*>(this));
    }
    template<typename Trait, typename Rt>
    auto as_trait(std::function<Rt(Trait const&)> fn, Rt rv) const -> Rt
    {
        if (not has_trait<Trait>()) {
            return rv;
        }

        return fn(*dynamic_cast<Trait const*>(this));
    }
    template<typename Trait> auto as_trait(Cell const& cell) const -> Cell
    {
        if (not has_trait<Trait>()) {
            throw std::runtime_error{type_name()
                                     + " does not implement required trait"};
        }

        auto const& tr = *dynamic_cast<Trait const*>(this);
        return tr(Trait::tag, cell);
    }
    template<typename Trait> auto as_trait() const -> Trait const&
    {
        return *dynamic_cast<Trait const*>(this);
    }

    template<typename Trait> auto has_trait() const -> bool
    {
        return (dynamic_cast<Trait const*>(this) != nullptr);
    }

    auto pointer_to() -> std::unique_ptr<class Pointer>;
};
}  // namespace viua::vm::types

#endif
