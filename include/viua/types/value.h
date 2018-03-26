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

#ifndef VIUA_TYPES_VALUE_H
#define VIUA_TYPES_VALUE_H

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>


namespace viua {
namespace process {
class Process;
}

namespace types {
class Pointer;

class Value {
    friend class Pointer;
    std::vector<Pointer*> pointers;

  public:
    /** Basic interface of a Value.
     *
     *  Derived objects are expected to override this methods, but in case they
     * do not Value provides safe defaults.
     */
    virtual std::string type() const;
    virtual std::string str() const;
    virtual std::string repr() const;
    virtual bool boolean() const;

    virtual std::unique_ptr<Pointer> pointer(const viua::process::Process*);

    virtual std::vector<std::string> bases() const;
    virtual std::vector<std::string> inheritancechain() const;

    virtual std::unique_ptr<Value> copy() const = 0;

    Value() = default;
    virtual ~Value();
};
}  // namespace types
}  // namespace viua


#endif
