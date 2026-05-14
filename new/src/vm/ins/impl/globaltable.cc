/*
 *  Copyright (C) 2021-2025 Marek Marecki
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

#include <stdint.h>

#include <viua/arch/arch.h>
#include <viua/vm/ins.h>


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;

auto execute(
    GTS const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const key   = immutable_proxy(stack, op.instruction.out)
                           .get<register_type::atom_type>();
    auto const value = immutable_proxy(stack, op.instruction.in);

    if (not key.has_value()) {
        throw abort_execution{ stack, "invalid type used as global table key" };
    }

    if (value.is_void()) {
        stack.proc->globals.erase(key->key);
    } else {
        stack.proc->globals[key->key] = value.target;
    }
}
auto execute(
    GTL const op,
    Stack& stack,
    ip_type const) -> void
{
    auto value     = mutable_proxy(stack, op.instruction.out);
    auto const key = immutable_proxy(stack, op.instruction.in)
                         .get<register_type::atom_type>();

    if (not key.has_value()) {
        throw abort_execution{ stack, "invalid type used as global table key" };
    }

    auto& gt = stack.proc->globals;
    if (not gt.contains(key->key)) {
        throw abort_execution{ stack,
                               ("key not present in globals table: "
                                + stack.proc->atoms[key->key]) };
    }

    value = gt[key->key];
}
}  // namespace viua::vm::ins
