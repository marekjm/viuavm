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
    COPY const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const in = immutable_proxy(stack, op.instruction.in);
    // FIXME Improve static analysis run by the assembler to catch calling
    // functions with "holes" in the frame's argument register set eg,
    //
    //      frame $2.a
    //      copy $0.a, ...
    //      call void, foo
    //
    // This leaves $1.p in the callee empty, and could lead to the error
    // reported below, the "cannot copy a void".
    //
    // FIXME Improve static analysis run by the assembler to catch
    // copying, or moving out of, a void. This is a non-sensical operation.
    // Sure, one could argue that the following code could be a valid way of
    // erasing a register $1.l:
    //
    //      copy $1.l, void     ; use void directly
    //      copy $1.l, $123.l   ; use a register that happens to be empty as the
    //                          ; source (this one sounds like an error)
    //
    // But. There already is a standard way of erasing a register ie, the
    // "delete" pseudoinstruction:
    //
    //      delete $1.l
    //
    // which expands to:
    //
    //      move void, $1.l
    //
    // So, since there is a canonical way of erasing a register, I see no need
    // of the copy-a-void being a valid operation.
    if (in.target.is_void()) {
        throw abort_execution{ stack, "cannot copy a void" };
    }
    mutable_proxy(stack, op.instruction.out) = in;
}
auto execute(
    MOVE const op,
    Stack& stack,
    ip_type const) -> void
{
    auto in = mutable_proxy(stack, op.instruction.in);
    if (in.target->is_void()) {
        throw abort_execution{ stack, "cannot move out of void" };
    }

    mutable_proxy(stack, op.instruction.out) = std::move(*in.target);
    in.reset();
}
auto execute(
    SWAP const op,
    Stack& stack,
    ip_type const) -> void
{
    auto lhs = mutable_proxy(stack, op.instruction.in);
    auto rhs = mutable_proxy(stack, op.instruction.out);
    std::swap(*lhs.target, *rhs.target);
}
}  // namespace viua::vm::ins
