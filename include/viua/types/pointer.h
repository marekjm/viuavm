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

#ifndef VIUA_TYPES_POINTER_H
#define VIUA_TYPES_POINTER_H

#include <vector>

#include <viua/kernel/frame.h>
#include <viua/types/value.h>


namespace viua {
namespace kernel {
class Kernel;
}

namespace types {
class Pointer : public Value {
    Value* points_to = nullptr;
    /*
     *  Pointer of origin is a parallelism-safety token.
     *  Viua asserts that pointers can be dereferenced only
     *  inside the process that spawned them -- otherwise
     *  there is no way to ensure that the pointer is still valid
     *  without sophisticated sycnhronisation schemes.
     *  Also, since the VM employs shared-nothing concurrency,
     *  pointer dereferences outside of the process that taken the
     *  pointer should be illegal by definition (even if the access
     *  could be made safe).
     */
    viua::process::PID const origin;


  public:
    constexpr static auto type_name = "Pointer";

    /*
     * Check if the pointer is expired, and set its state appropriately. Return
     * the new state.
     */
    auto expired(viua::process::Process const&) -> bool;

    /*
     * Authenticate the pointer for use in a process. This causes the pointer to
     * become invalid if it is used outside of its process of origin.
     */
    auto authenticate(viua::process::PID const pid) -> void;

    /*
     * Guarded (via to() function) and unguarded (via of() function) access to
     * the pointee.
     *
     * The to() function requires a process that the value would be used in, and
     * may set the pointer's state to expired if it's not the pointer's process
     * of origin.
     */
    auto to(viua::process::Process const&) -> Value*;
    auto of() const -> Value*;

    auto str() const -> std::string override;

    auto type() const -> std::string override;
    auto boolean() const -> bool override;

    auto copy() const -> std::unique_ptr<Value> override;
    auto expire() -> void override;

    Pointer(viua::process::PID const);
    Pointer(Value* t, viua::process::PID const);
    ~Pointer() override;
};
}  // namespace types
}  // namespace viua


#endif
