/*
 *  Copyright (C) 2017, 2019 Marek Marecki
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

#ifndef VIUA_TYPES_STRUCT_H
#define VIUA_TYPES_STRUCT_H

#include <map>
#include <string>
#include <vector>

#include <viua/types/value.h>


namespace viua { namespace types {
class Struct : public Value {
    /** A generic object class.
     *
     *  This type is used internally inside the VM.
     */
  private:
    std::map<std::string, std::unique_ptr<Value>> attributes;

  public:
    constexpr static auto type_name = "Struct";

    auto type() const -> std::string override;
    auto boolean() const -> bool override;

    auto str() const -> std::string override;
    auto repr() const -> std::string override;

    virtual auto insert(std::string const& key, std::unique_ptr<Value> value)
        -> void;
    virtual auto remove(std::string const& key) -> std::unique_ptr<Value>;
    virtual auto at(std::string const& key) -> Value*;
    virtual auto at(std::string const& key) const -> Value const*;
    virtual auto keys() const -> std::vector<std::string>;

    auto copy() const -> std::unique_ptr<Value> override;

    ~Struct() override = default;
};
}}  // namespace viua::types


#endif
