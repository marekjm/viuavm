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
    BITSHL const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() << *v);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
auto execute(
    BITSHR const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() >> *v);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
auto execute(
    BITASHR const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        auto const tmp = static_cast<int64_t>(*lhs.get<uint64_t>());
        out            = static_cast<uint64_t>(tmp >> *v);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
auto execute(
    BITROL const,
    Stack&,
    ip_type const) -> void
{}
auto execute(
    BITROR const,
    Stack&,
    ip_type const) -> void
{}
auto execute(
    BITAND const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() & *v);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
auto execute(
    BITOR const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() | *v);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
auto execute(
    BITXOR const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() ^ *v);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
auto execute(
    BITNOT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const in  = immutable_proxy(stack, op.instruction.in);
    if (auto const v = in.get<uint64_t>(); v) {
        out = ~*v;
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for bit operation" };
}
}  // namespace viua::vm::ins
