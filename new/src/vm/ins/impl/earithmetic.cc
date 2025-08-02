/*
 *  Copyright (C) 2025 Marek Marecki
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

#include <print>

#include <viua/arch/arch.h>
#include <viua/support/binarith.hh>
#include <viua/vm/ins.h>


namespace {
using viua::vm::Stack;

auto calculate_add(
    Stack& stack,
    int64_t const lhs,
    int64_t const rhs) -> int64_t
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        signed_type{ extend(arithmetic_type{ lhs }, arithmetic_width) };
    auto const arithmetic_rhs =
        signed_type{ extend(arithmetic_type{ rhs }, arithmetic_width) };

    switch (stack.proc->arithmetic_style) {
        using enum viua::vm::Process::Arithmetic_style;
        case Wrapping:
            {
                using namespace viua::arithmetic::fixed;
                return arithmetic_lhs + arithmetic_rhs;
            }
        case Trapping:
            {
                using namespace viua::arithmetic::fixed;
                return arithmetic_lhs + arithmetic_rhs;
            }
        case Saturating:
            {
                using namespace viua::arithmetic::saturating;
                return arithmetic_lhs + arithmetic_rhs;
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for addition"
    };
}

auto calculate_add(
    Stack&,
    uint64_t const lhs,
    uint64_t const rhs) -> int64_t
{
    return (lhs + rhs);
}

auto calculate_sub(
    Stack&,
    int64_t const lhs,
    int64_t const rhs) -> int64_t
{
    return (lhs - rhs);
}

auto calculate_sub(
    Stack&,
    uint64_t const lhs,
    uint64_t const rhs) -> int64_t
{
    return (lhs - rhs);
}
}  // namespace


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;

auto execute(
    EARITHMETICSTYLE const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const in  = immutable_proxy(stack, op.instruction.in);

    auto const current_style = stack.proc->arithmetic_style;
    out                      = static_cast<uint64_t>(current_style);

    if (in.is_void()) {
        return;
    }

    auto const new_style = in.cast_to<uint64_t>();
    if (not new_style.has_value()) {
        throw abort_execution{ stack,
                               "invalid input operand for earithmeticstyle in "
                                   + op.instruction.in.to_string() };
    }

    auto const ns = static_cast<viua::vm::Process::Arithmetic_style>(
        static_cast<uint64_t>(*new_style));
    switch (ns) {
        using enum viua::vm::Process::Arithmetic_style;
        case Wrapping:
        case Trapping:
        case Saturating:
            stack.proc->arithmetic_style = ns;
            break;
        default:
            throw abort_execution{ stack,
                                   "unknown style for earithmeticstyle: "
                                       + std::to_string(
                                           static_cast<uint64_t>(*new_style)) };
    }
}

auto execute(
    EARITHMETICWIDTH const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const in  = immutable_proxy(stack, op.instruction.in);

    auto const current_width = stack.proc->arithmetic_width;
    out                      = static_cast<uint64_t>(current_width);

    if (in.is_void()) {
        return;
    }

    auto const new_width = in.cast_to<uint64_t>();
    if (not new_width.has_value()) {
        throw abort_execution{ stack,
                               "invalid input operand for earithmeticwidth in "
                                   + op.instruction.in.to_string() };
    }

    stack.proc->arithmetic_width = static_cast<uint8_t>(*new_width);
}

auto execute(
    STDADD const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = calculate_add(stack, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = calculate_add(stack, *lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}

auto execute(
    STDSUB const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = calculate_sub(stack, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = calculate_sub(stack, *lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}

auto execute(
    STDMUL const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    using fn_type = std::multiplies<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}

auto execute(
    STDDIV const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    using fn_type = std::divides<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}
}  // namespace viua::vm::ins
