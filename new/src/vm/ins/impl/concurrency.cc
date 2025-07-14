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
    ACTOR const op,
    Stack& stack,
    ip_type const) -> void
{
    auto fn_addr = size_t{};
    if (auto fn = mutable_proxy(stack, op.instruction.in)
                      .get<register_type::pointer_type>();
        fn) {
        fn_addr = fn->ptr;
        fn.reset();
    } else {
        throw abort_execution{ stack,
                               "invalid in operand to actor instruction" };
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{ stack, "invalid IP after asynchronous call" };
    }

    auto const fr_entry = (fn_addr / sizeof(viua::arch::instruction_type));

    auto const pid = stack.proc->core->spawn("", fr_entry);
    auto dst       = mutable_proxy(stack, op.instruction.out);
    dst            = pid.get();
}
auto execute(
    SELF const op,
    Stack& stack,
    ip_type const) -> void
{
    mutable_proxy(stack, op.instruction.out) = stack.proc->pid.get();
}
}  // namespace viua::vm::ins
