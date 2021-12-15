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

#include <map>
#include <vector>

#include <viua/vm/types/traits.h>
#include <viua/vm/types/value.h>


/*
 * The following types are primitive, and usually unboxed. However, due to
 * certain constraints (ie, pointers) they need boxed variants. A boxed variant
 * is undistinguishable from the programmer's point of view without expliting
 * the instrucpection interfaces provided by Viua.
 */
namespace viua::vm::types {
struct Signed_integer
        : Value
        , traits::To_string {
    using value_type = int64_t;
    value_type value{};

    inline Signed_integer(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
struct Unsigned_integer
        : Value
        , traits::To_string {
    using value_type = uint64_t;
    value_type value{};

    inline Unsigned_integer(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
struct Float_single
        : Value
        , traits::To_string {
    using value_type = float;
    value_type value{};

    inline Float_single(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
struct Float_double
        : Value
        , traits::To_string {
    using value_type = double;
    value_type value{};

    inline Float_double(value_type const v) : value{v}
    {}

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
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
struct Ref
        : Value
        , traits::To_string {
    using value_type = Value*;
    value_type value;

    inline explicit Ref(value_type const p) : value{p}
    {}

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};

struct String
        : Value
        , traits::To_string
        , traits::Bool
        , traits::Cmp
        , traits::Plus {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
    explicit operator bool() const override;
    auto operator()(traits::Plus::tag_type const, Cell const&) const
        -> Cell override;
    auto operator() (traits::Cmp const&, Cell_view const&) const -> std::strong_ordering override;
};

struct Atom
        : Value
        , traits::To_string
        , traits::Eq {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
    auto operator() (traits::Eq const&, Cell_view const&) const -> std::partial_ordering override;
};

struct Struct
        : Value
        , traits::To_string {
    using value_type = Cell;
    using key_type   = std::string;

    std::map<key_type, value_type> values;

    auto insert(key_type const, value_type&&) -> void;
    auto at(key_type const) -> value_type&;
    auto at(key_type const) const -> value_type const&;
    auto remove(key_type const) -> value_type;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};

struct Buffer
        : Value
        , traits::To_string {
    using value_type = Cell;

    std::vector<value_type> values;
    using size_type = decltype(values)::size_type;

    auto push(value_type&&) -> void;
    auto at(size_type) -> value_type&;
    auto at(size_type) const -> value_type const&;
    auto pop(size_type) -> value_type;
    auto size() const -> size_type;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
}  // namespace viua::vm::types

#endif
