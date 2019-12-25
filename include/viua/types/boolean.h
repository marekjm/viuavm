/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#ifndef VIUA_TYPES_BOOLEAN_H
#define VIUA_TYPES_BOOLEAN_H

#include <sstream>
#include <string>
#include <viua/types/value.h>


namespace viua { namespace types {
class Boolean : public viua::types::Value {
    /** Boolean object.
     *
     *  This type is used to hold true and false values.
     */
    bool b;

  public:
    static std::string const type_name;

    auto type() const -> std::string override;
    auto str() const -> std::string override;
    auto boolean() const -> bool override;

    auto value() -> bool&;

    auto copy() const -> std::unique_ptr<Value> override;

    Boolean(bool const v = false);
};
}}  // namespace viua::types


#endif
