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

#ifndef VIUA_VM_TYPES_H
#define VIUA_VM_TYPES_H

#include <viua/vm/types/value.h>
#include <viua/vm/types/traits.h>

namespace viua::vm::types {
struct String
        : Value
        , traits::To_string
        , traits::Bool
        , traits::Eq
        , traits::Plus {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
    operator bool () const override;
    auto operator() (traits::Plus::tag_type const, Register_cell const&) const -> Register_cell override;
    auto operator() (traits::Eq::tag_type const, Register_cell const&) const -> Register_cell override;
};

struct Atom
        : Value
        , traits::To_string
        , traits::Eq {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
    auto operator() (traits::Eq::tag_type const, Register_cell const&) const -> Register_cell override;
};

struct Struct
        : Value
        , traits::To_string {
    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};

struct Buffer
        : Value
        , traits::To_string {
    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
}  // namespace viua::vm::types

#endif
