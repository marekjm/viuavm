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
    EQ const op,
    Stack& stack,
    ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64  = lhs.holds<register_type::int_type>();
    auto const lhs_u64  = lhs.holds<register_type::uint_type>();
    auto const lhs_f32  = lhs.holds<register_type::float_type>();
    auto const lhs_f64  = lhs.holds<register_type::double_type>();
    auto const lhs_ptr  = lhs.holds<register_type::pointer_type>();
    auto const lhs_atom = lhs.holds<register_type::atom_type>();
    auto const lhs_pid  = lhs.holds<register_type::pid_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        cmp_result = (*lhs.get<int64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        cmp_result = (*lhs.get<uint64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        cmp_result = (*lhs.get<float>() <=> *v);
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        cmp_result = (*lhs.get<double>() <=> *v);
    }
    if (auto const v = rhs.get<register_type::pointer_type>(); lhs_ptr and v) {
        cmp_result = (lhs.get<register_type::pointer_type>()->ptr <=> v->ptr);
    }
    if (auto const v = rhs.get<register_type::atom_type>(); lhs_atom and v) {
        cmp_result = (lhs.get<register_type::atom_type>()->key <=> v->key);
    }
    if (auto const v = rhs.get<register_type::pid_type>(); lhs_pid and v) {
        auto const lhs_pid = *lhs.get<register_type::pid_type>();
        auto const rhs_pid = *v;
        cmp_result =
            memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr))
                ? std::partial_ordering::less /* whatever, just not equivalent
                                               */
                : std::partial_ordering::equivalent;
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ stack, "cannot eq unordered values" };
    }

    out = (cmp_result == 0);
}
auto execute(
    LT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    if (lhs.holds<void>()) {
        throw abort_execution{ stack, "invalid read from empty register" };
    }
    if (rhs.holds<void>()) {
        throw abort_execution{ stack, "invalid read from empty register" };
    }

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();
    auto const lhs_ptr = lhs.holds<register_type::pointer_type>();
    auto const lhs_pid = lhs.holds<register_type::pid_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        cmp_result = (*lhs.get<int64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        cmp_result = (*lhs.get<uint64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        cmp_result = (*lhs.get<float>() <=> *v);
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        cmp_result = (*lhs.get<double>() <=> *v);
    }
    if (auto const v = rhs.get<register_type::pointer_type>(); lhs_ptr and v) {
        cmp_result = (lhs.get<register_type::pointer_type>()->ptr <=> v->ptr);
    }
    if (auto const v = rhs.get<register_type::pid_type>(); lhs_pid and v) {
        auto const lhs_pid = *lhs.get<register_type::pid_type>();
        auto const rhs_pid = *v;
        auto const r =
            memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr));
        if (r < 0) {
            cmp_result = std::partial_ordering::less;
        } else if (r > 0) {
            cmp_result = std::partial_ordering::greater;
        } else {
            cmp_result = std::partial_ordering::equivalent;
        }
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ stack, "cannot lt unordered values" };
    }

    out = (cmp_result < 0);
}
auto execute(
    GT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();
    auto const lhs_ptr = lhs.holds<register_type::pointer_type>();
    auto const lhs_pid = lhs.holds<register_type::pid_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        cmp_result = (*lhs.get<int64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        cmp_result = (*lhs.get<uint64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        cmp_result = (*lhs.get<float>() <=> *v);
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        cmp_result = (*lhs.get<double>() <=> *v);
    }
    if (auto const v = rhs.get<register_type::pointer_type>(); lhs_ptr and v) {
        cmp_result = (lhs.get<register_type::pointer_type>()->ptr <=> v->ptr);
    }
    if (auto const v = rhs.get<register_type::pid_type>(); lhs_pid and v) {
        auto const lhs_pid = *lhs.get<register_type::pid_type>();
        auto const rhs_pid = *v;
        auto const r =
            memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr));
        if (r < 0) {
            cmp_result = std::partial_ordering::less;
        } else if (r > 0) {
            cmp_result = std::partial_ordering::greater;
        } else {
            cmp_result = std::partial_ordering::equivalent;
        }
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ stack, "cannot lt unordered values" };
    }

    out = (cmp_result > 0);
}
auto execute(
    CMP const op,
    Stack& stack,
    ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64  = lhs.holds<register_type::int_type>();
    auto const lhs_u64  = lhs.holds<register_type::uint_type>();
    auto const lhs_f32  = lhs.holds<register_type::float_type>();
    auto const lhs_f64  = lhs.holds<register_type::double_type>();
    auto const lhs_ptr  = lhs.holds<register_type::pointer_type>();
    auto const lhs_atom = lhs.holds<register_type::atom_type>();
    auto const lhs_pid  = lhs.holds<register_type::pid_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        cmp_result = (*lhs.get<int64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        cmp_result = (*lhs.get<uint64_t>() <=> *v);
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        cmp_result = (*lhs.get<float>() <=> *v);
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        cmp_result = (*lhs.get<double>() <=> *v);
    }
    if (auto const v = rhs.get<register_type::pointer_type>(); lhs_ptr and v) {
        cmp_result = (lhs.get<register_type::pointer_type>()->ptr <=> v->ptr);
    }
    if (auto const v = rhs.get<register_type::atom_type>(); lhs_atom and v) {
        cmp_result = (lhs.get<register_type::atom_type>()->key <=> v->key);
    }
    if (auto const v = rhs.get<register_type::pid_type>(); lhs_pid and v) {
        auto const lhs_pid = *lhs.get<register_type::pid_type>();
        auto const rhs_pid = *v;
        auto const r =
            memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr));
        if (r < 0) {
            cmp_result = std::partial_ordering::less;
        } else if (r > 0) {
            cmp_result = std::partial_ordering::greater;
        } else {
            cmp_result = std::partial_ordering::equivalent;
        }
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ stack, "cannot cmp unordered values" };
    }

    out = (cmp_result < 0) ? -1 : (0 < cmp_result) ? 1 : 0;
}
auto execute(
    AND const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs).cast_to<bool>();
    auto const rhs = immutable_proxy(stack, op.instruction.rhs).cast_to<bool>();

    if (lhs.has_value() and rhs.has_value()) {
        out = static_cast<uint64_t>(*lhs and *rhs);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for and operation" };

    /*
     * This is the old implementation which was moving the operand that
     * determined the value of the expression into the output register.
     * I think this still may be desirable behaviour, but maybe the instruction
     * can be named differently eg, andmove, to send a stronger signal that it
     * is not a simple logical op.
     *
    if (auto l = lhs.boxed_of<Bool>(); l.has_value()) {
        auto const use_lhs = static_cast<bool>(l.value().get());
        auto const use_lhs = not lhs.boxed_value().as_trait<Bool, bool>(
            [](Bool const& v) -> bool { return static_cast<bool>(v); }, false);
    } else {
        auto const use_lhs = (cast_to<uint64_t>(lhs) == 0);
        out                = use_lhs ? std::move(lhs) : std::move(rhs);
    }
    */
}
auto execute(
    OR const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs).cast_to<bool>();
    auto const rhs = immutable_proxy(stack, op.instruction.rhs).cast_to<bool>();

    if (lhs.has_value() and rhs.has_value()) {
        out = static_cast<uint64_t>(*lhs or *rhs);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand types for or operation" };
}
auto execute(
    NOT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const in  = immutable_proxy(stack, op.instruction.in).cast_to<bool>();
    if (in.has_value()) {
        out = static_cast<uint64_t>(not *in);
        return;
    }

    throw abort_execution{ stack,
                           "unsupported operand type for not operation" };
}
}  // namespace viua::vm::ins
