/*
 *  Copyright (C) 2017, 2020 Marek Marecki
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

#ifndef VIUA_TYPES_BITS_H
#define VIUA_TYPES_BITS_H

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <viua/types/value.h>


namespace viua { namespace types {
class Bits : public viua::types::Value {
    std::vector<bool> underlying_array;

  public:
    using size_type = decltype(underlying_array)::size_type;
    auto size() const -> size_type;

    auto at(size_type) const -> bool;
    auto set(size_type, bool const = true) -> bool;
    auto flip(size_type) -> bool;

    auto clear() -> void;

    auto shl(size_type) -> std::unique_ptr<Bits>;
    auto shr(size_type, bool const) -> std::unique_ptr<Bits>;
    auto shr(size_type) -> std::unique_ptr<Bits>;
    auto ashl(size_type) -> std::unique_ptr<Bits>;
    auto ashr(size_type) -> std::unique_ptr<Bits>;

    auto rol(size_type) -> void;
    auto ror(size_type) -> void;

    auto inverted() const -> std::unique_ptr<Bits>;

    auto increment() -> void;
    auto decrement() -> void;

    auto wrapadd(Bits const&) const -> std::unique_ptr<Bits>;
    auto wrapsub(Bits const&) const -> std::unique_ptr<Bits>;
    auto wrapmul(Bits const&) const -> std::unique_ptr<Bits>;
    auto wrapdiv(Bits const&) const -> std::unique_ptr<Bits>;

    auto checked_signed_increment() -> void;
    auto checked_signed_decrement() -> void;
    auto checked_signed_add(Bits const&) const -> std::unique_ptr<Bits>;
    auto checked_signed_sub(Bits const&) const -> std::unique_ptr<Bits>;
    auto checked_signed_mul(Bits const&) const -> std::unique_ptr<Bits>;
    auto checked_signed_div(Bits const&) const -> std::unique_ptr<Bits>;

    auto saturating_signed_increment() -> void;
    auto saturating_signed_decrement() -> void;
    auto saturating_signed_add(Bits const&) const -> std::unique_ptr<Bits>;
    auto saturating_signed_sub(Bits const&) const -> std::unique_ptr<Bits>;
    auto saturating_signed_mul(Bits const&) const -> std::unique_ptr<Bits>;
    auto saturating_signed_div(Bits const&) const -> std::unique_ptr<Bits>;

    auto operator==(Bits const&) const -> bool;
    auto operator<(Bits const&) const -> bool;
    auto operator<=(Bits const&) const -> bool;
    auto operator>(Bits const&) const -> bool;
    auto operator>=(Bits const&) const -> bool;

    auto operator&(Bits const&) const -> std::unique_ptr<Bits>;
    auto operator|(Bits const&) const -> std::unique_ptr<Bits>;
    auto operator^(Bits const&) const -> std::unique_ptr<Bits>;

    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    Bits(std::vector<bool>&&);
    Bits(std::vector<bool> const&);
    Bits(const size_type);
    Bits(std::vector<uint8_t> const);
};
}}  // namespace viua::types


#endif
