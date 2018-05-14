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

#ifndef VIUA_TYPES_ATOM_H
#define VIUA_TYPES_ATOM_H

#pragma once

#include <viua/types/value.h>


namespace viua { namespace types {
class Atom : public Value {
    std::string const value;

  public:
    static std::string const type_name;

    virtual std::string type() const override;
    virtual bool boolean() const override;

    virtual std::string str() const override;
    virtual std::string repr() const override;

    operator std::string() const;
    auto operator==(Atom const&) const -> bool;

    virtual std::unique_ptr<Value> copy() const override;

    Atom(std::string);
    ~Atom() override = default;
};
}}  // namespace viua::types


#endif
