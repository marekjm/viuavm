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

#include <stdint.h>

#include <viua/vm/types/traits.h>
#include <viua/vm/types/value.h>


/*
 * The following types are primitive, and usually unboxed. However, due to
 * certain constraints (ie, pointers) they need boxed variants. A boxed variant
 * is undistinguishable from the programmer's point of view without expliting
 * the instrucpection interfaces provided by Viua.
 */
namespace viua::vm::types {
struct Signed_integer : Value {
    using value_type = int64_t;
    value_type value{};

    inline Signed_integer(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
};
struct Unsigned_integer : Value {
    using value_type = uint64_t;
    value_type value{};

    inline Unsigned_integer(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
};
struct Float_single : Value {
    using value_type = float;
    value_type value{};

    inline Float_single(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
};
struct Float_double : Value {
    using value_type = double;
    value_type value{};

    inline Float_double(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
};
}  // namespace viua::vm::types

/*
 * The following types are primitive, but boxed. This is due to the fact that a
 * register has fixed size while they all have a size defined by the programmer
 * who is writing the code to be run by Viua virtual machine.
 *
 * Their varying size makes it impossible to store them in a fixed-size memory
 * area which is a register.
 */
namespace viua::vm::types {
struct Pointer : Value {
    using value_type = Value*;
    value_type value;

    inline Pointer(value_type const p) : value{p}
    {}

    auto type_name() const -> std::string override;
};

struct String
        : Value
        , traits::To_string
        , traits::Bool
        , traits::Eq
        , traits::Plus {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
    operator bool() const override;
    auto operator()(traits::Plus::tag_type const, Register_cell const&) const
        -> Register_cell override;
    auto operator()(traits::Eq::tag_type const, Register_cell const&) const
        -> Register_cell override;
};

struct Atom
        : Value
        , traits::To_string
        , traits::Eq {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
    auto operator()(traits::Eq::tag_type const, Register_cell const&) const
        -> Register_cell override;
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
    std::vector<Value_cell> values;
    using size_type = decltype(values)::size_type;

    auto push(Value_cell&&) -> void;
    auto pop(size_type) -> Value_cell;
    auto size() const -> size_type;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
}  // namespace viua::vm::types

#endif
