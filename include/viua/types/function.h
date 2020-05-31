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

#ifndef VIUA_TYPE_FUNCTION_H
#define VIUA_TYPE_FUNCTION_H

#include <string>

#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/registerset.h>
#include <viua/types/value.h>


namespace viua { namespace types {
class Function : public Value {
  public:
    static std::string const type_name;

    std::string function_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;

    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    virtual std::string name() const;

    // FIXME: implement real dtor
    Function(std::string const& = "");
    virtual ~Function();
};
}}  // namespace viua::types


#endif
