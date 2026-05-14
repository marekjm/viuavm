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
    FRAME const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const index = op.instruction.out.index;
    auto const rs    = op.instruction.out.set;

    auto capacity = viua::arch::register_index_type{};
    switch (rs) {
        case viua::arch::REGISTER_SET::LOCAL:
            if (auto v =
                    immutable_proxy(stack, op.instruction.out).get<uint64_t>();
                v) {
                capacity = static_cast<viua::arch::register_index_type>(*v);
            } else {
                throw abort_execution{
                    stack, "dynamic args count must be an unsigned integer"
                };
            }
            break;
        case viua::arch::REGISTER_SET::ARGUMENT:
            capacity = index;
            break;
        default:
            throw abort_execution{
                stack,
                "args count must come from local or argument register set"
            };
    }

    stack.args = std::vector<register_type>(capacity);
}

auto execute(
    CALL const op,
    Stack& stack,
    ip_type const) -> ip_type
{
    auto fn_addr = size_t{};
    if (auto fn = mutable_proxy(stack, op.instruction.in)
                      .get<register_type::pointer_type>();
        fn) {
        fn_addr = fn->ptr;
        fn.reset();
    } else {
        throw abort_execution{ stack,
                               "invalid src operand to call instruction" };
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{ stack, "invalid IP after synchronous call" };
    }

    /*
     * Save:
     *
     *  - frame pointer ie, pointer to where the frames memory area begins
     *  - stack break ie, pointer to where next unallocated pointer is on the
     *    stack memory
     *
     * They should be restored when the frame is popped.
     */
    stack.frames.back().saved.fp   = stack.proc->frame_pointer;
    stack.frames.back().saved.sbrk = stack.proc->stack_break;

    auto const fr_return = (stack.ip + 1);
    auto const fr_entry  = (stack.proc->module.ip_base
                            + (fn_addr / sizeof(viua::arch::instruction_type)));

    stack.frames.emplace_back(
        viua::arch::MAX_REGISTER_INDEX, fr_entry, fr_return);
    stack.frames.back().parameters = std::move(stack.args);
    stack.frames.back().result_to  = op.instruction.out;

    /*
     * Set the frame pointer to stack break. Usually, one of the first
     * instructions in the callee is AMA which will increase the stack break
     * giving the function some memory to work with.
     */
    stack.proc->frame_pointer      = stack.proc->stack_break;
    stack.frames.back().saved.fp   = stack.proc->frame_pointer;
    stack.frames.back().saved.sbrk = stack.proc->stack_break;

    return fr_entry;
}

auto execute(
    RETURN const op,
    Stack& stack,
    ip_type const) -> ip_type
{
    auto fr = std::move(stack.frames.back());
    stack.frames.pop_back();

    if (stack.frames.empty()) {
        return fr.return_address;
    }

    if (auto const rt = fr.result_to; not rt.is_void()) {
        // FIXME detect trying to return a dereference and throw an exception.
        // The following code is invalid and should be rejected:
        //
        //      return *1
        //
        // It would be best if the static analysis phase during assembly caught
        // such errors and refused to produce the ELF output.
        auto const ret = immutable_proxy(fr, op.instruction.out, stack);
        if (ret.holds<void>()) {
            throw abort_execution{
                stack, "return value requested from function returning void"
            };
        }

        mutable_proxy(stack, rt) = ret;
    }

    stack.proc->frame_pointer = stack.frames.back().saved.fp;
    stack.proc->stack_break   = stack.frames.back().saved.sbrk;
    stack.proc->prune_pointers();

    return fr.return_address;
}

auto execute(
    IF const op,
    Stack& stack,
    ip_type const ip) -> ip_type
{
    auto const condition = immutable_proxy(stack, op.instruction.out);

    auto const take_branch =
        (condition.holds<void>() or *condition.cast_to<bool>());

    auto target_offset = size_t{};
    if (auto jmp = mutable_proxy(stack, op.instruction.in)
                       .get<register_type::pointer_type>();
        jmp) {
        target_offset = jmp->ptr;
        jmp.reset();
    } else {
        throw abort_execution{ stack, "invalid src operand to if instruction" };
    }

    auto const target_addr =
        target_offset / sizeof(viua::arch::instruction_type);
    auto const target =
        take_branch ? (stack.proc->module.ip_base + target_addr) : (ip + 1);

    return target;
}

auto execute(
    MOVEIF const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const condition = immutable_proxy(stack, op.instruction.lhs);

    auto const move_lhs =
        (not condition.holds<void>()) and *condition.cast_to<bool>();

    if ((not move_lhs)
        and immutable_proxy(stack, op.instruction.rhs).holds<void>()) {
        throw abort_execution{ stack,
                               "invalid rhs operand to moveif instruction" };
    }

    auto move_from = move_lhs ? mutable_proxy(stack, op.instruction.lhs)
                              : mutable_proxy(stack, op.instruction.rhs);

    mutable_proxy(stack, op.instruction.out) = std::move(*move_from.target);
}
}  // namespace viua::vm::ins
