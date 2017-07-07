/*
 *  Copyright (C) 2017 Marek Marecki
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

#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <viua/types/value.h>


namespace viua {
    namespace types {
        class Bits : public viua::types::Value {
            std::vector<bool> underlying_array;

            Bits(const std::vector<bool>&);

          public:
            using size_type = decltype(underlying_array)::size_type;

            auto at(size_type) const -> bool;
            auto set(size_type, const bool = true) -> bool;
            auto flip(size_type) -> bool;

            auto clear() -> void;

            auto shl(size_type) -> Bits;
            auto shr(size_type) -> Bits;
            auto ashl(size_type) -> Bits;
            auto ashr(size_type) -> Bits;

            auto operator|(const Bits&) const -> Bits;

            static const std::string type_name;

            std::string type() const override;
            std::string str() const override;
            bool boolean() const override;

            std::unique_ptr<Value> copy() const override;

            Bits(const size_type);
        };
    }
}


#endif
