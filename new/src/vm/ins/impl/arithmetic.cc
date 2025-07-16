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
             ? viua::support::sign_extend<immediate_type>(
                   op.instruction.immediate)
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
        auto const n =
            typename Op::template functor_type<int64_t>{}(*v, immediate);
        out = n;
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
}  // namespace viua::vm::ins
