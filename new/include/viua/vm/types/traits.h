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

#ifndef VIUA_VM_TYPES_TRAITS_H
#define VIUA_VM_TYPES_TRAITS_H

#include <string>
#include <string_view>

#include <viua/support/string.h>
#include <viua/vm/types/value.h>


namespace viua::vm::types::traits {
#define VIUA_TRAIT_TAG()          \
    struct tag_type {             \
    };                            \
    static constexpr tag_type tag \
    {}

#define VIUA_TRAIT_BODY(Trait)                                            \
    VIUA_TRAIT_TAG();                                                     \
    virtual auto operator()(tag_type const, Cell const&) const->Cell = 0; \
    virtual ~Trait()                                                 = default

#define VIUA_TRAIT(Trait)       \
    struct Trait {              \
        VIUA_TRAIT_BODY(Trait); \
    }

struct To_string {
    virtual auto to_string() const -> std::string = 0;
    virtual ~To_string();

    static auto quote_and_escape(std::string_view const sv) -> std::string
    {
        return viua::support::string::quoted(sv);
    }
};

struct Bool {
    virtual operator bool() const = 0;
    virtual ~Bool();
};

struct Copy {
    virtual auto copy() const -> std::unique_ptr<Value> = 0;
    virtual ~Copy()                                     = default;
};

VIUA_TRAIT(Eq);
VIUA_TRAIT(Lt);
VIUA_TRAIT(Gt);

struct Cmp {
    static constexpr int64_t CMP_EQ = 0;
    static constexpr int64_t CMP_GT = 1;
    static constexpr int64_t CMP_LT = -1;

    virtual auto cmp(Value const&) const -> int64_t = 0;
    virtual ~Cmp();
};

VIUA_TRAIT(Plus);

#undef VIUA_TRAIT
#undef VIUA_TRAIT_BODY
#undef VIUA_TRAIT_TAG
}  // namespace viua::vm::types::traits

#endif
