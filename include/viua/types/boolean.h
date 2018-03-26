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

#pragma once

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
    static const std::string type_name;

    std::string type() const override;
    std::string str() const override;
    bool boolean() const override;

    bool& value();

    virtual std::vector<std::string> bases() const override;
    virtual std::vector<std::string> inheritancechain() const override;

    std::unique_ptr<Value> copy() const override;

    Boolean(bool v = false);
};
}}  // namespace viua::types


#endif
