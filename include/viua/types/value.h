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

#include <memory>
#include <set>
#include <string>

#include <viua/pid.h>


namespace viua {
namespace process {
class Process;
}

namespace types {
class Pointer;

class Value {
    class viua::process::Process* pointered = nullptr;

  public:
    /*
     * Basic interface of a Value.
     *
     * Derived types are expected to override these methods. In case they do
     * not, Value provides safe defaults.
     * not
     * do not Value provides safe defaults.
     */
    virtual auto type() const -> std::string;
    virtual auto str() const -> std::string;
    virtual auto repr() const -> std::string;
    virtual auto boolean() const -> bool;

    /*
     * Called before a value is sent to another process. For types that should
     * not be usable outside of their original process it should set the value
     * to some sane, invalid state; if such a state is impossible to construct
     * the function should throw an exception.
     *
     * This is currently only useful for pointers (as they are the only value
     * that SHOULD NOT be exchanged by processes).
     */
    virtual auto expire() -> void;
    virtual auto pointer(viua::process::Process* const)
        -> std::unique_ptr<Pointer>;

    virtual auto copy() const -> std::unique_ptr<Value> = 0;

    Value() = default;
    virtual ~Value();
};
}  // namespace types
}  // namespace viua


#endif
