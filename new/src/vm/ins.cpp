/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#include <endian.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/support/fdstream.h>
#include <viua/vm/ins.h>

namespace viua {
extern viua::support::fdstream TRACE_STREAM;
}

namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;

auto execute(
    ADD const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();
    auto const lhs_ptr = lhs.holds<register_type::pointer_type>();

    using fn_type = std::plus<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        out = fn_type{}(*lhs.get<float>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        out = fn_type{}(*lhs.get<double>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_ptr and v) {
        auto const offset = *v;

        using Pt               = register_type::pointer_type;
        auto const old_address = lhs.get<Pt>()->ptr;
        auto const new_address = (old_address + offset);

        auto const old_ptr     = stack.proc->get_pointer(old_address);
        auto const ffi_pointer = old_ptr->foreign;
        if ((not ffi_pointer) and offset >= old_ptr->size) {
            auto o = std::ostringstream{};
            o << "illegal offset of " << offset << " bytes into a region of "
              << old_ptr->size << " byte(s)";
            throw abort_execution{ stack, o.str() };
        }

        auto new_ptr    = Pointer{};
        new_ptr.ptr     = new_address;
        new_ptr.size    = (old_ptr->size - offset);
        new_ptr.foreign = old_ptr->foreign;
        new_ptr.parent  = old_ptr->ptr;
        stack.proc->record_pointer(new_ptr);

        out = Pt{ new_address };

        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for arithmetic operation"
    };
}
auto execute(
    SUB const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();

    using fn_type = std::minus<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        out = fn_type{}(*lhs.get<float>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        out = fn_type{}(*lhs.get<double>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for arithmetic operation"
    };
}
auto execute(
    MUL const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();

    using fn_type = std::multiplies<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        out = fn_type{}(*lhs.get<float>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        out = fn_type{}(*lhs.get<double>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for arithmetic operation"
    };
}
auto execute(
    DIV const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();

    using fn_type = std::divides<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<float>(); lhs_f32 and v) {
        out = fn_type{}(*lhs.get<float>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<double>(); lhs_f64 and v) {
        out = fn_type{}(*lhs.get<double>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for arithmetic operation"
    };
}
auto execute(
    MOD const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    using fn_type = std::modulus<>;

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = fn_type{}(*lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = fn_type{}(*lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for arithmetic operation"
    };
}

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

auto execute(
    COPY const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const in = immutable_proxy(stack, op.instruction.in);
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

auto execute(
    ATOM const op,
    Stack& stack,
    ip_type const) -> void
{
    auto target = mutable_proxy(stack, op.instruction.out);

    auto const& strtab     = *stack.proc->strtab;
    auto const data_offset = target.get<uint64_t>();
    if (not data_offset.has_value()) {
        throw abort_execution{ stack, "invalid operand for atom constructor" };
    }
    auto const data_size = [&strtab, data_offset]() -> uint64_t
    {
        auto const size_offset = (*data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strtab[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto const data_address =
        reinterpret_cast<char const*>(&strtab[0] + *data_offset);
    auto const key         = reinterpret_cast<uint64_t>(data_address);
    auto value             = std::string{ data_address, data_size };
    stack.proc->atoms[key] = std::move(value);
    target                 = register_type::atom_type{ key };
}

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
                               "invalid in operand to call instruction" };
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
     * Set the frame pointer to stack break to. Usually, one of the first
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
    LUI const op,
    Stack& stack,
    ip_type const) -> void
{
    auto out = mutable_proxy(stack, op.instruction.out);
    out      = static_cast<int64_t>(op.instruction.immediate) << 32;
}
auto execute(
    LUIU const op,
    Stack& stack,
    ip_type const) -> void
{
    auto out = mutable_proxy(stack, op.instruction.out);
    out      = static_cast<uint64_t>(op.instruction.immediate) << 32;
}
auto execute(
    LLI const op,
    Stack& stack,
    ip_type const) -> void
{
    auto out = mutable_proxy(stack, op.instruction.out);

    constexpr auto LOW_32  = uint64_t{ 0x00'00'00'00'ff'ff'ff'ff };
    constexpr auto HIGH_32 = uint64_t{ 0xff'ff'ff'ff'00'00'00'00 };

    if (auto const v = out.get<uint64_t>(); v) {
        auto const high = (HIGH_32 & *v);
        auto const low  = (LOW_32 & op.instruction.immediate);
        out             = (high | low);
    } else if (auto const v = out.get<int64_t>(); v) {
        auto const high = (HIGH_32 & *v);
        auto const low  = (LOW_32 & op.instruction.immediate);
        out             = static_cast<int64_t>(high | low);
    } else {
        throw abort_execution{ stack,
                               "unsupported operand type for lli operation: "
                                   + std::string{ out.type_name() } };
    }
}
auto execute(
    CAST const op,
    Stack& stack,
    ip_type const) -> void
{
    auto target = mutable_proxy(stack, op.instruction.out);

    using viua::arch::FUNDAMENTAL_TYPES;
    auto const desired_type =
        static_cast<FUNDAMENTAL_TYPES>(op.instruction.immediate);

    if (target.is_void()) {
        throw abort_execution{ stack, "cannot cast void" };
    }

    auto& slot = target.to()->get();
    if (not slot.holds<Register::undefined_type>()) {
        throw abort_execution{ stack, "invalid cast" };
    }

    switch (desired_type) {
        using enum viua::arch::FUNDAMENTAL_TYPES;
        case INT:
            slot.convert_undefined_to<Register::int_type>();
            break;
        case UINT:
            slot.convert_undefined_to<Register::uint_type>();
            break;
        case FLOAT32:
            slot.convert_undefined_to<Register::float_type>();
            break;
        case FLOAT64:
            slot.convert_undefined_to<Register::double_type>();
            break;
        case POINTER:
            slot.convert_undefined_to<Register::pointer_type>();
            break;
        case ATOM:
            slot.convert_undefined_to<Register::atom_type>();
            break;
        case PID:
            slot.convert_undefined_to<Register::pid_type>();
            break;
        case VOID:
        case UNDEFINED:
            throw abort_execution{ stack, "invalid cast" };
    }
}
auto execute(
    ARODP const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const& strtab     = *stack.proc->strtab;
    auto const data_offset = op.instruction.immediate;

    auto const pointer_address = const_cast<uint8_t*>(&strtab[data_offset]);
    mutable_proxy(stack, op.instruction.out) = register_type::pointer_type{
        reinterpret_cast<uintptr_t>(pointer_address)
    };

    auto pointer_info    = Pointer{};
    pointer_info.ptr     = reinterpret_cast<uintptr_t>(pointer_address);
    pointer_info.foreign = true;
    stack.proc->record_pointer(pointer_info);
}
auto execute(
    ATXTP const op,
    Stack& stack,
    ip_type const) -> void
{
    mutable_proxy(stack, op.instruction.out) =
        register_type::pointer_type{ op.instruction.immediate };
}

auto execute(
    FLOAT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto out = mutable_proxy(stack, op.instruction.out);

    auto tmp = uint32_t{};
    memcpy(&tmp, &op.instruction.immediate, sizeof(tmp));

    auto v = float{};
    memcpy(&v, &tmp, sizeof(v));

    out = v;
}
auto execute(
    DOUBLE const op,
    Stack& stack,
    ip_type const) -> void
{
    auto target = mutable_proxy(stack, op.instruction.out);

    auto const& strtab = *stack.proc->strtab;

    auto const data_offset = target.get<uint64_t>();
    if (not data_offset.has_value()) {
        throw abort_execution{ stack, "invalid operand" };
    }

    auto const data_size = [&strtab, data_offset]() -> uint64_t
    {
        auto const size_offset = (*data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strtab[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint64_t{};
    memcpy(
        &tmp, (&stack.proc->strtab->operator[](0) + *data_offset), data_size);
    tmp = le64toh(tmp);

    auto v = double{};
    memcpy(&v, &tmp, data_size);

    target = v;
}

template<typename Op>
auto execute_arithmetic_immediate_op(
    Op const op,
    Stack& stack) -> void
{
    auto out = mutable_proxy(stack, op.instruction.out);
    auto in  = immutable_proxy(stack, op.instruction.in);

    constexpr auto const signed_immediate =
        std::is_signed_v<typename Op::value_type>;
    using immediate_type =
        typename std::conditional<signed_immediate, int64_t, uint64_t>::type;
    auto const immediate =
        (signed_immediate
             ? static_cast<immediate_type>(
                   static_cast<int32_t>(op.instruction.immediate << 8) >> 8)
             : static_cast<immediate_type>(op.instruction.immediate));

    if (in.template holds<void>()) {
        out =
            typename Op::template functor_type<immediate_type>{}(0, immediate);
        return;
    }
    if (auto const v = in.template get<uint64_t>(); v) {
        out = typename Op::template functor_type<uint64_t>{}(*v, immediate);
        return;
    }
    if (auto const v = in.template get<int64_t>(); v) {
        out = typename Op::template functor_type<int64_t>{}(*v, immediate);
        return;
    }
    if (auto const v = in.template get<float>(); v) {
        out = typename Op::template functor_type<float>{}(
            *v, static_cast<float>(immediate));
        return;
    }
    if (auto const v = in.template get<double>(); v) {
        out = typename Op::template functor_type<double>{}(
            *v, static_cast<double>(immediate));
        return;
    }
    if (auto const v = in.template get<register_type::pointer_type>();
        v and not signed_immediate) {
        auto const r =
            typename Op::template functor_type<uint64_t>{}(v->ptr, immediate);
        out = register_type::pointer_type{ static_cast<uint64_t>(r) };
        return;
    }

    throw abort_execution{
        stack,
        "unsupported lhs operand type for immediate arithmetic operation: "
            + std::string{ in.type_name() }
    };
}
auto execute(
    ADDI const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    ADDIU const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    SUBI const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    SUBIU const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    MULI const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    MULIU const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    DIVI const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(
    DIVIU const op,
    Stack& stack,
    ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
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
        throw abort_execution{ stack, "invalid in operand to if instruction" };
    }

    auto const target_addr =
        target_offset / sizeof(viua::arch::instruction_type);
    auto const target =
        take_branch ? (stack.proc->module.ip_base + target_addr) : (ip + 1);

    return target;
}

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

auto execute(
    GTS const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const key = immutable_proxy(stack, op.instruction.out)
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

auto execute(
    SM const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const base = immutable_proxy(stack, op.instruction.in)
                          .get<register_type::pointer_type>();

    auto const unit      = op.instruction.spec;
    auto const copy_size = (1u << unit);
    auto const offset    = (op.instruction.immediate * copy_size);

    if (not base.has_value()) {
        throw abort_execution{ stack,
                               "invalid base operand for memory instruction" };
    }

    auto const pointer_info = stack.proc->get_pointer(base->ptr);
    if (not pointer_info.has_value()) {
        auto o = std::ostringstream{};
        o << "unknown pointer: ";
        o << std::hex << std::setfill('0') << std::setw(16) << base->ptr;
        throw abort_execution{ stack, o.str() };
    }

    if (offset >= pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal offset of " << offset << " bytes into a region of "
          << pointer_info->size << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }
    if ((offset + copy_size) > pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal store of " << copy_size << " bytes into a region of "
          << (pointer_info->size - offset) << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }

    auto const user_addr = base->ptr + offset;
    auto const addr      = stack.proc->memory_at(user_addr);
    if (addr == nullptr) {
        auto o = std::ostringstream{};
        o << "invalid store address: ";
        o << std::hex << std::setfill('0') << std::setw(16) << user_addr;
        throw abort_execution{ stack, o.str() };
    }

    auto const in = immutable_proxy(stack, op.instruction.out);
    if (in.holds<void>()) {
        throw abort_execution{ stack,
                               "invalid in operand for memory instruction" };
    }
    auto const buf = in.target.as_memory();
    memcpy(addr, buf.data(), copy_size);
}
auto execute(
    LM const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const base = immutable_proxy(stack, op.instruction.in)
                          .get<register_type::pointer_type>();

    auto const unit      = op.instruction.spec;
    auto const copy_size = (1u << unit);
    auto const offset    = (op.instruction.immediate * copy_size);

    if (not base.has_value()) {
        throw abort_execution{ stack,
                               "invalid base operand for memory instruction" };
    }

    auto const pointer_info = stack.proc->get_pointer(base->ptr);
    if (not pointer_info.has_value()) {
        auto o = std::ostringstream{};
        o << "unknown pointer: ";
        o << std::hex << std::setfill('0') << std::setw(16) << base->ptr;
        throw abort_execution{ stack, o.str() };
    }

    auto const ffi_pointer = pointer_info->foreign;
    if ((not ffi_pointer) and offset >= pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal offset of " << offset << " bytes into a region of "
          << pointer_info->size << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }
    if ((not ffi_pointer) and (offset + copy_size) > pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal load of " << copy_size << " bytes from a region of "
          << (pointer_info->size - offset) << " byte(s)";
        throw abort_execution{ stack, o.str() };
    }

    auto const user_addr = base->ptr + offset;
    auto const addr      = ffi_pointer ? reinterpret_cast<uint8_t*>(base->ptr)
                                       : stack.proc->memory_at(user_addr);
    if (addr == nullptr) {
        auto o = std::ostringstream{};
        o << "invalid load address: ";
        o << std::hex << std::setfill('0') << std::setw(16) << user_addr;
        throw abort_execution{ stack, o.str() };
    }

    auto out = mutable_proxy(stack, op.instruction.out);
    auto raw = register_type::undefined_type{};
    memcpy(raw.data(), addr, copy_size);
    out                         = raw;
    out.to()->get().loaded_size = copy_size;
}
auto execute(
    AD const,
    Stack&,
    ip_type const) -> void
{}
auto execute(
    PTR const,
    Stack&,
    ip_type const) -> void
{}
}  // namespace viua::vm::ins
