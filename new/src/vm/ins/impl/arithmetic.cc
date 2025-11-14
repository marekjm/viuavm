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

#include <cmath>
#include <optional>
#include <type_traits>

#include <viua/arch/arch.h>
#include <viua/support/binarith.hh>
#include <viua/vm/ins.h>


namespace {
using viua::vm::Stack;

template<typename T,
         typename A = std::conditional<std::is_signed_v<T>,
                                       viua::arithmetic::signed_type,
                                       viua::arithmetic::unsigned_type>::type>
auto make_arithmetic(
    T const v,
    size_t const width,
    viua::arch::opcode_type const style) -> std::optional<A>
{
    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            return viua::arithmetic::fixed::make_arithmetic(v, width);
        case ARITHMETIC_STYLE_TRAP:
            return viua::arithmetic::fixed::make_arithmetic(v, width);
        case ARITHMETIC_STYLE_SATURATE:
            return viua::arithmetic::saturating::make_arithmetic(v, width);
    }
    return std::nullopt;
}

namespace impl {
template<typename T,
         typename A = std::conditional<std::is_signed_v<T>,
                                       viua::arithmetic::signed_type,
                                       viua::arithmetic::unsigned_type>::type>
auto add(
    Stack& stack,
    viua::arch::opcode_type const style,
    T const lhs,
    T const rhs) -> T
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        make_arithmetic(lhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();
    auto const arithmetic_rhs =
        make_arithmetic(rhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs + arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs + arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<T>(arithmetic_lhs + arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for addition"
    };
}

template<typename T,
         typename A = std::conditional<std::is_signed_v<T>,
                                       viua::arithmetic::signed_type,
                                       viua::arithmetic::unsigned_type>::type>
auto sub(
    Stack& stack,
    viua::arch::opcode_type const style,
    T const lhs,
    T const rhs) -> T
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        make_arithmetic(lhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();
    auto const arithmetic_rhs =
        make_arithmetic(rhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs - arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs - arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<T>(arithmetic_lhs - arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for subtraction"
    };
}

template<typename T,
         typename A = std::conditional<std::is_signed_v<T>,
                                       viua::arithmetic::signed_type,
                                       viua::arithmetic::unsigned_type>::type>
auto mul(
    Stack& stack,
    viua::arch::opcode_type const style,
    T const lhs,
    T const rhs) -> T
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        make_arithmetic(lhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();
    auto const arithmetic_rhs =
        make_arithmetic(rhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs * arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs * arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<T>(arithmetic_lhs * arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for multiplication"
    };
}

template<typename T,
         typename A = std::conditional<std::is_signed_v<T>,
                                       viua::arithmetic::signed_type,
                                       viua::arithmetic::unsigned_type>::type>
auto div(
    Stack& stack,
    viua::arch::opcode_type const style,
    T const lhs,
    T const rhs) -> T
{
    using namespace viua::arithmetic;

    auto const arithmetic_width = stack.proc->arithmetic_width;
    auto const arithmetic_lhs =
        make_arithmetic(lhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();
    auto const arithmetic_rhs =
        make_arithmetic(rhs, arithmetic_width, style)
            .or_else(
                [&stack] -> std::optional<A>
                {
                    throw viua::vm::abort_execution{
                        stack, "cannot make arithmetic value with bad style"
                    };
                })
            .value();

    switch (style) {
        using namespace viua::arch::ops::OPCODE_FLAGS;
        case ARITHMETIC_STYLE_WRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs / arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_TRAP:
            {
                using namespace viua::arithmetic::fixed;
                return static_cast<T>(arithmetic_lhs / arithmetic_rhs);
            }
        case ARITHMETIC_STYLE_SATURATE:
            {
                using namespace viua::arithmetic::saturating;
                return static_cast<T>(arithmetic_lhs / arithmetic_rhs);
            }
    }

    throw viua::vm::abort_execution{
        stack, "broken environment: bad arithmetic style for division"
    };
}
}  // namespace impl
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

    auto const val = in.cast_to<uint64_t>();
    if (not val.has_value()) {
        throw abort_execution{ stack,
                               "invalid input operand for earithmeticwidth in "
                                   + op.instruction.in.to_string() };
    }

    auto const candidate_width = static_cast<uint8_t>(*val);
    stack.proc->arithmetic_width = candidate_width
        ? candidate_width
        : 64u;
}
}  // namespace viua::vm::ins


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;

auto native_add(
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
auto styled_add(
    ADD const op,
    viua::arch::opcode_type const style,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = impl::add(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = impl::add(stack, style, *lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}
auto execute(
    ADD const op,
    Stack& stack,
    ip_type const ip) -> void
{
    auto const style = viua::carve_flags_out(op.instruction.opcode);
    if (style == viua::arch::ops::OPCODE_FLAGS::ARITHMETIC_STYLE_NATIVE) {
        return native_add(op, stack, ip);
    } else {
        return styled_add(op, style, stack, ip);
    }
}

auto native_sub(
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
auto styled_sub(
    SUB const op,
    viua::arch::opcode_type const style,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = impl::sub(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = impl::sub(stack, style, *lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}
auto execute(
    SUB const op,
    Stack& stack,
    ip_type const ip) -> void
{
    auto const style = viua::carve_flags_out(op.instruction.opcode);
    if (style == viua::arch::ops::OPCODE_FLAGS::ARITHMETIC_STYLE_NATIVE) {
        return native_sub(op, stack, ip);
    } else {
        return styled_sub(op, style, stack, ip);
    }
}

auto native_mul(
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
auto styled_mul(
    MUL const op,
    viua::arch::opcode_type const style,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = impl::mul(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = impl::mul(stack, style, *lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}
auto execute(
    MUL const op,
    Stack& stack,
    ip_type const ip) -> void
{
    auto const style = viua::carve_flags_out(op.instruction.opcode);
    if (style == viua::arch::ops::OPCODE_FLAGS::ARITHMETIC_STYLE_NATIVE) {
        return native_mul(op, stack, ip);
    } else {
        return styled_mul(op, style, stack, ip);
    }
}

auto native_div(
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
auto styled_div(
    DIV const op,
    viua::arch::opcode_type const style,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.lhs);
    auto const rhs = immutable_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();

    if (auto const v = rhs.cast_to<int64_t>(); lhs_i64 and v) {
        out = impl::div(stack, style, *lhs.get<int64_t>(), *v);
        return;
    }
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = impl::div(stack, style, *lhs.get<uint64_t>(), *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for styled arithmetic operation"
    };
}
auto execute(
    DIV const op,
    Stack& stack,
    ip_type const ip) -> void
{
    auto const style = viua::carve_flags_out(op.instruction.opcode);
    if (style == viua::arch::ops::OPCODE_FLAGS::ARITHMETIC_STYLE_NATIVE) {
        return native_div(op, stack, ip);
    } else {
        return styled_div(op, style, stack, ip);
    }
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
    SQRT const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const in = immutable_proxy(stack, op.instruction.in);

    if (auto const v = in.get<register_type::double_type>(); v) {
        out = std::sqrt(*v);
        return;
    }
    if (auto const v = in.get<register_type::float_type>(); v) {
        out = std::sqrt(*v);
        return;
    }
    if (auto const v = in.cast_to<register_type::double_type>(); v) {
        out = std::sqrt(*v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for sqrt"
    };
}

template<typename Op>
auto native_arithmetic_immediate_op(
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
template<typename Op>
auto styled_arithmetic_immediate_op(
    Op const op,
    viua::arch::opcode_type const style,
    Stack& stack) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const lhs = immutable_proxy(stack, op.instruction.in);

    constexpr auto const signed_immediate =
        std::is_signed_v<typename Op::value_type>;
    using immediate_type =
        typename std::conditional<signed_immediate, int64_t, uint64_t>::type;
    auto const immediate =
        (signed_immediate
             ? viua::support::sign_extend<immediate_type>(
                   op.instruction.immediate)
             : static_cast<immediate_type>(op.instruction.immediate));

    auto const lhs_i64 = lhs.template holds<register_type::int_type>();
    auto const lhs_u64 = lhs.template holds<register_type::uint_type>();

    if (lhs_i64) {
        out = impl::add(stack,
                        style,
                        *lhs.template get<int64_t>(),
                        static_cast<int64_t>(immediate));
        return;
    }
    if (lhs_u64) {
        out = impl::add(stack,
                        style,
                        *lhs.template get<uint64_t>(),
                        static_cast<uint64_t>(immediate));
        return;
    }

    throw abort_execution{
        stack,
        "unsupported operand types for styled immediate arithmetic operation"
    };
}

template<typename Op>
auto execute_arithmetic_immediate_op(
    Op const op,
    Stack& stack) -> void
{
    auto const style = viua::carve_flags_out(op.instruction.opcode);

    if (style == viua::arch::ops::OPCODE_FLAGS::ARITHMETIC_STYLE_NATIVE) {
        return native_arithmetic_immediate_op(op, stack);
    } else {
        return styled_arithmetic_immediate_op(op, style, stack);
    }
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
