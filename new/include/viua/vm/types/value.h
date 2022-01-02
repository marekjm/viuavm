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

#ifndef VIUA_VM_TYPES_VALUE_H
#define VIUA_VM_TYPES_VALUE_H

#include <stdint.h>

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include <viua/vm/core/cell.h>


namespace viua::vm::types {
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

    auto reference_to() -> std::unique_ptr<class Ref>;
};
}  // namespace viua::vm::types

#endif
