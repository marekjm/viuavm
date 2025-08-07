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
    viua::arch::opcode_type const style,
    int64_t const lhs,
    int64_t const rhs) -> int64_t
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        signed_type{ extend(arithmetic_type{ lhs }, arithmetic_width) };
    auto const arithmetic_rhs =
        signed_type{ extend(arithmetic_type{ rhs }, arithmetic_width) };

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<int64_t>(arithmetic_lhs + arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<int64_t>(arithmetic_lhs + arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<int64_t>(arithmetic_lhs + arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for addition"
    };
}

auto calculate_add(
    Stack&,
    viua::arch::opcode_type const,
    uint64_t const lhs,
    uint64_t const rhs) -> uint64_t
{
    return (lhs + rhs);
}

auto calculate_sub(
    Stack& stack,
    viua::arch::opcode_type const style,
    int64_t const lhs,
    int64_t const rhs) -> int64_t
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        signed_type{ extend(arithmetic_type{ lhs }, arithmetic_width) };
    auto const arithmetic_rhs =
        signed_type{ extend(arithmetic_type{ rhs }, arithmetic_width) };

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<int64_t>(arithmetic_lhs - arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<int64_t>(arithmetic_lhs - arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<int64_t>(arithmetic_lhs - arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for subtraction"
    };
}

auto calculate_sub(
    Stack&,
    viua::arch::opcode_type const,
    uint64_t const lhs,
    uint64_t const rhs) -> uint64_t
{
    return (lhs - rhs);
}

auto calculate_mul(
    Stack& stack,
    viua::arch::opcode_type const style,
    int64_t const lhs,
    int64_t const rhs) -> int64_t
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        signed_type{ extend(arithmetic_type{ lhs }, arithmetic_width) };
    auto const arithmetic_rhs =
        signed_type{ extend(arithmetic_type{ rhs }, arithmetic_width) };

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<int64_t>(arithmetic_lhs * arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<int64_t>(arithmetic_lhs * arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<int64_t>(arithmetic_lhs * arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for subtraction"
    };
}

auto calculate_mul(
    Stack&,
    viua::arch::opcode_type const,
    uint64_t const lhs,
    uint64_t const rhs) -> uint64_t
{
    return (lhs * rhs);
}
}  // namespace


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;

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

    auto const style = op.instruction.opcode & viua::arch::ops::OPCODE_FLG_MASK;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = calculate_add(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = calculate_add(stack, style, *lhs.get<uint64_t>(), *v);
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

    auto const style = op.instruction.opcode & viua::arch::ops::OPCODE_FLG_MASK;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = calculate_sub(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = calculate_sub(stack, style, *lhs.get<uint64_t>(), *v);
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

    auto const style = op.instruction.opcode & viua::arch::ops::OPCODE_FLG_MASK;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = calculate_mul(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = calculate_mul(stack, style, *lhs.get<uint64_t>(), *v);
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
