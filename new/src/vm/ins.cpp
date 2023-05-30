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

auto execute(viua::vm::Stack& stack,
             viua::arch::instruction_type const* const ip)
    -> viua::arch::instruction_type const*
{
    auto const raw = *ip;

    auto const opcode = static_cast<viua::arch::opcode_type>(
        raw & viua::arch::ops::OPCODE_MASK);
    auto const format = static_cast<viua::arch::ops::FORMAT>(
        opcode & viua::arch::ops::FORMAT_MASK);

    switch (format) {
        using viua::vm::ins::execute;
        using namespace viua::arch::ins;
        using enum viua::arch::ops::FORMAT;
    case T:
    {
        auto instruction = viua::arch::ops::T::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_T;
        switch (static_cast<OPCODE_T>(opcode)) {
#define Work(OP) case OPCODE_T::OP: execute(OP{instruction}, stack, ip); break
            Work(ADD);
            Work(SUB);
            Work(MUL);
            Work(DIV);
            Work(MOD);
            Work(BITSHL);
            Work(BITSHR);
            Work(BITASHR);
            Work(BITROL);
            Work(BITROR);
            Work(BITAND);
            Work(BITOR);
            Work(BITXOR);
            Work(EQ);
            Work(GT);
            Work(LT);
            Work(CMP);
            Work(AND);
            Work(OR);
            Work(IO_SUBMIT);
            Work(IO_WAIT);
            Work(IO_SHUTDOWN);
            Work(IO_CTL);
#undef Work
        }
        break;
    }
    case S:
    {
        auto instruction = viua::arch::ops::S::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_S;
        switch (static_cast<OPCODE_S>(opcode)) {
#define Work(OP) case OPCODE_S::OP: execute(OP{instruction}, stack, ip); break
#define Flow(OP) case OPCODE_S::OP: return execute(OP{instruction}, stack, ip)
            Work(FRAME);
            Flow(RETURN);
            Work(ATOM);
            Work(STRING);
            Work(FLOAT);
            Work(DOUBLE);
            Work(SELF);
#undef Work
#undef Flow
        }
        break;
    }
    case E:
    {
        auto instruction = viua::arch::ops::E::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_E;
        switch (static_cast<OPCODE_E>(opcode)) {
#define Work(OP) case OPCODE_E::OP: execute(OP{instruction}, stack, ip); break
            Work(LUI);
            Work(LUIU);
#undef Work
        }
        break;
    }
    case R:
    {
        auto instruction = viua::arch::ops::R::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_R;
        switch (static_cast<OPCODE_R>(opcode)) {
#define Work(OP) case OPCODE_R::OP: execute(OP{instruction}, stack, ip); break
            Work(ADDI);
            Work(ADDIU);
            Work(SUBI);
            Work(SUBIU);
            Work(MULI);
            Work(MULIU);
            Work(DIVI);
            Work(DIVIU);
#undef Work
        }
        break;
    }
    case N:
    {
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << viua::arch::ops::to_string(opcode)
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_N;
        switch (static_cast<OPCODE_N>(opcode)) {
        case OPCODE_N::NOOP:
            break;
        case OPCODE_N::HALT:
            return nullptr;
        case OPCODE_N::EBREAK:
            execute(EBREAK{viua::arch::ops::N::decode(raw)}, stack, ip);
            break;
        case OPCODE_N::ECALL:
            execute(ECALL{viua::arch::ops::N::decode(raw)}, stack, ip);
            break;
        }
        break;
    }
    case M:
    {
        auto instruction = viua::arch::ops::M::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_M;
        switch (static_cast<OPCODE_M>(opcode)) {
#define Work(OP) case OPCODE_M::OP: execute(OP{instruction}, stack, ip); break
            Work(SM);
            Work(LM);
            Work(AA);
            Work(AD);
            Work(PTR);
#undef Work
        }
        break;
    }
    case D:
    {
        auto instruction = viua::arch::ops::D::decode(raw);
        if constexpr (VIUA_TRACE_CYCLES) {
            viua::TRACE_STREAM << "    " << instruction.to_string()
                               << viua::TRACE_STREAM.endl;
        }

        using viua::arch::ops::OPCODE_D;
        switch (static_cast<OPCODE_D>(opcode)) {
#define Work(OP) case OPCODE_D::OP: execute(OP{instruction}, stack, ip); break
#define Flow(OP) case OPCODE_D::OP: return execute(OP{instruction}, stack, ip)
            /*
             * Call is a special instruction. It transfers the IP to a
             * semi-random location, instead of just increasing it to the next
             * unit.
             *
             * This is why we return here, and not use the default behaviour for
             * most of the other instructions.
             */
            Flow(CALL);
            Work(BITNOT);
            Work(NOT);
            Work(COPY);
            Work(MOVE);
            Work(SWAP);
            Work(REF);
            /*
             * If is a special instruction. It transfers IP to a semi-random
             * location instead of just increasing it to the next unit. This is
             * why the return is used instead of break.
             */
            Flow(IF);
            Work(IO_PEEK);
            Work(ACTOR);
#undef Work
#undef Flow
        }
        break;
    }
    case F:
        std::cerr << "unimplemented instruction: "
                  << viua::arch::ops::to_string(opcode) << "\n";
        return nullptr;
    }

    return (ip + 1);
}

auto save_proxy(Stack& stack, access_type const a) -> Save_proxy
{
    if (not a.direct) {
        throw abort_execution{stack, "dereferences are not implemented"};
    }

    switch (a.set) {
        using enum viua::arch::REGISTER_SET;
        case VOID:
            return {nullptr};
        case LOCAL:
            return {&stack.frames.back().registers.at(a.index)};
        case PARAMETER:
            return {&stack.frames.back().parameters.at(a.index)};
        case ARGUMENT:
            return {&stack.args.at(a.index)};
        default:
            throw abort_execution{stack, "illegal write access to register " + a.to_string()};
    }
}
auto fetch_proxy(Stack& stack, access_type const a) -> Fetch_proxy
{
    if (not a.direct) {
        throw abort_execution{stack, "dereferences are not implemented"};
    }

    static register_type const void_placeholder;
    switch (a.set) {
        using enum viua::arch::REGISTER_SET;
        case VOID:
            return void_placeholder;
        case LOCAL:
            return stack.frames.back().registers.at(a.index);
        case PARAMETER:
            return stack.frames.back().parameters.at(a.index);
        case ARGUMENT:
            return stack.args.at(a.index);
        default:
            throw abort_execution{stack, "illegal read access to register " + a.to_string()};
    }
}
auto fetch_proxy(Frame& frame, access_type const a, Stack const& stack) -> Fetch_proxy
{
    if (not a.direct) {
        throw abort_execution{stack, "dereferences are not implemented"};
    }

    static register_type const void_placeholder;
    switch (a.set) {
        using enum viua::arch::REGISTER_SET;
        case VOID:
            return void_placeholder;
        case LOCAL:
            return frame.registers.at(a.index);
        case PARAMETER:
            return frame.parameters.at(a.index);
        default:
            throw abort_execution{stack, "illegal read access to register " + a.to_string()};
    }
}

#if 0
using viua::vm::types::Cell_view;
template<typename Op, typename Lhs, typename Boxed_lhs>
auto execute_arithmetic_op_impl(ip_type const ip,
                                Proxy& out,
                                Cell_view& lhs,
                                Cell_view& rhs,
                                std::string_view const tn) -> void
{
    /*
     * Instead of casting the lhs operand we could just get it. However, by
     * always casting we can use the same code for unboxed and boxed variants.
     * The difference is not visible on the ISA level, but we have to deal with
     * the hairy details on the implementation level.
     */
    auto r = typename Op::functor_type{}(cast_to<Lhs>(lhs), cast_to<Lhs>(rhs));
    store_impl<Boxed_lhs>(ip, out, std::move(r), tn);
}
template<typename Op>
auto execute_arithmetic_op(Op const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;

    auto const holds_i64 =
        (lhs.template holds<int64_t>() or lhs.template holds<Signed_integer>());
    auto const holds_u64 = (lhs.template holds<uint64_t>()
                            or lhs.template holds<Unsigned_integer>());
    auto const holds_f32 =
        (lhs.template holds<float>() or lhs.template holds<Float_single>());
    auto const holds_f64 =
        (lhs.template holds<double>() or lhs.template holds<Float_double>());

    if (holds_i64) {
        execute_arithmetic_op_impl<Op, int64_t, Signed_integer>(
            ip, out, lhs, rhs, "i64");
    } else if (holds_u64) {
        execute_arithmetic_op_impl<Op, uint64_t, Unsigned_integer>(
            ip, out, lhs, rhs, "u64");
    } else if (holds_f32) {
        execute_arithmetic_op_impl<Op, float, Float_single>(
            ip, out, lhs, rhs, "fl");
    } else if (holds_f64) {
        execute_arithmetic_op_impl<Op, double, Float_double>(
            ip, out, lhs, rhs, "db");
    } else {
        throw abort_execution{
            ip, "unsupported operand types for arithmetic operation"};
    }
}
#endif

auto execute(ADD const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

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

        using Pt = register_type::pointer_type;
        auto const old_address = lhs.get<Pt>()->ptr;
        auto const new_address = (old_address + offset);

        auto const old_ptr = stack.proc->get_pointer(old_address);
        if (offset >= old_ptr->size) {
            auto o = std::ostringstream{};
            o << "illegal offset of " << offset
                << " bytes into a region of "
                << old_ptr->size << " byte(s)";
            throw abort_execution{stack, o.str()};
        }

        auto new_ptr = Pointer{};
        new_ptr.ptr = new_address;
        new_ptr.size = (old_ptr->size - offset);
        new_ptr.parent = old_ptr->ptr;
        stack.proc->record_pointer(new_ptr);

        out = Pt{new_address};

        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for arithmetic operation"};
}
auto execute(SUB const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

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
        stack, "unsupported operand types for arithmetic operation"};
}
auto execute(MUL const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

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
        stack, "unsupported operand types for arithmetic operation"};
}
auto execute(DIV const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

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
        stack, "unsupported operand types for arithmetic operation"};
}
auto execute(MOD const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

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
        stack, "unsupported operand types for arithmetic operation"};
}

auto execute(BITSHL const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() << *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}
auto execute(BITSHR const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() >> *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}
auto execute(BITASHR const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        auto const tmp = static_cast<int64_t>(*lhs.get<uint64_t>());
        out = static_cast<uint64_t>(tmp >> *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}
auto execute(BITROL const, Stack&, ip_type const) -> void
{}
auto execute(BITROR const, Stack&, ip_type const) -> void
{}
auto execute(BITAND const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() & *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}
auto execute(BITOR const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() | *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}
auto execute(BITXOR const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    if (auto const v = rhs.cast_to<uint64_t>(); lhs_u64 and v) {
        out = (*lhs.get<uint64_t>() ^ *v);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}
auto execute(BITNOT const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const in = fetch_proxy(stack, op.instruction.in);
    if (auto const v = in.get<uint64_t>(); v) {
        out = ~*v;
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for bit operation"};
}

auto execute(EQ const op, Stack& stack, ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();
    auto const lhs_ptr = lhs.holds<register_type::pointer_type>();
    auto const lhs_atom = lhs.holds<register_type::atom_type>();
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
    if (auto const v = rhs.get<register_type::atom_type>(); lhs_atom and v) {
        cmp_result = (lhs.get<register_type::atom_type>()->key <=> v->key);
    }
    if (auto const v = rhs.get<register_type::pid_type>(); lhs_pid and v) {
        auto const lhs_pid = *lhs.get<register_type::pid_type>();
        auto const rhs_pid = *v;
        cmp_result = memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr))
            ? std::partial_ordering::less /* whatever, just not equivalent */
            : std::partial_ordering::equivalent;
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{stack, "cannot eq unordered values"};
    }

    out = (cmp_result == 0);
}
auto execute(LT const op, Stack& stack, ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    if (lhs.holds<void>()) {
        throw abort_execution{stack, "invalid read from empty register"};
    }
    if (rhs.holds<void>()) {
        throw abort_execution{stack, "invalid read from empty register"};
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
        auto const r = memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr));
        if (r < 0) {
            cmp_result = std::partial_ordering::less;
        } else if (r > 0) {
            cmp_result = std::partial_ordering::greater;
        } else {
            cmp_result = std::partial_ordering::equivalent;
        }
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{stack, "cannot lt unordered values"};
    }

    out = (cmp_result < 0);
}
auto execute(GT const op, Stack& stack, ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

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
        auto const r = memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr));
        if (r < 0) {
            cmp_result = std::partial_ordering::less;
        } else if (r > 0) {
            cmp_result = std::partial_ordering::greater;
        } else {
            cmp_result = std::partial_ordering::equivalent;
        }
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{stack, "cannot lt unordered values"};
    }

    out = (cmp_result > 0);
}
auto execute(CMP const op, Stack& stack, ip_type const) -> void
{
    auto cmp_result = std::partial_ordering::unordered;

    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs);
    auto const rhs = fetch_proxy(stack, op.instruction.rhs);

    auto const lhs_i64 = lhs.holds<register_type::int_type>();
    auto const lhs_u64 = lhs.holds<register_type::uint_type>();
    auto const lhs_f32 = lhs.holds<register_type::float_type>();
    auto const lhs_f64 = lhs.holds<register_type::double_type>();
    auto const lhs_ptr = lhs.holds<register_type::pointer_type>();
    auto const lhs_atom = lhs.holds<register_type::atom_type>();
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
    if (auto const v = rhs.get<register_type::atom_type>(); lhs_atom and v) {
        cmp_result = (lhs.get<register_type::atom_type>()->key <=> v->key);
    }
    if (auto const v = rhs.get<register_type::pid_type>(); lhs_pid and v) {
        auto const lhs_pid = *lhs.get<register_type::pid_type>();
        auto const rhs_pid = *v;
        auto const r = memcmp(&lhs_pid.s6_addr, &rhs_pid.s6_addr, sizeof(lhs_pid.s6_addr));
        if (r < 0) {
            cmp_result = std::partial_ordering::less;
        } else if (r > 0) {
            cmp_result = std::partial_ordering::greater;
        } else {
            cmp_result = std::partial_ordering::equivalent;
        }
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{stack, "cannot cmp unordered values"};
    }

    out = (cmp_result < 0) ? -1 : (0 < cmp_result) ? 1 : 0;
}
auto execute(AND const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs).cast_to<bool>();
    auto const rhs = fetch_proxy(stack, op.instruction.rhs).cast_to<bool>();

    if (lhs.has_value() and rhs.has_value()) {
        out = static_cast<uint64_t>(*lhs and *rhs);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for and operation"};

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
auto execute(OR const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const lhs = fetch_proxy(stack, op.instruction.lhs).cast_to<bool>();
    auto const rhs = fetch_proxy(stack, op.instruction.rhs).cast_to<bool>();

    if (lhs.has_value() and rhs.has_value()) {
        out = static_cast<uint64_t>(*lhs or *rhs);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand types for or operation"};
}
auto execute(NOT const op, Stack& stack, ip_type const) -> void
{
    auto const out = save_proxy(stack, op.instruction.out);
    auto const in = fetch_proxy(stack, op.instruction.in).cast_to<bool>();
    if (in.has_value()) {
        out = static_cast<uint64_t>(not *in);
        return;
    }

    throw abort_execution{
        stack, "unsupported operand type for not operation"};
}

auto execute(COPY const op, Stack& stack, ip_type const) -> void
{
    auto const in = fetch_proxy(stack, op.instruction.in);
    save_proxy(stack, op.instruction.out) = in;
}
auto execute(MOVE const op, Stack& stack, ip_type const) -> void
{
    auto in = save_proxy(stack, op.instruction.in);
    if (in.target->is_void()) {
        throw abort_execution{stack, "cannot move out of void"};
    }

    save_proxy(stack, op.instruction.out) = std::move(*in.target);
    // FIXME save_proxy(stack, op.instruction.in, ip) = register_type::void_type{};
}
auto execute(SWAP const op, Stack& stack, ip_type const) -> void
{
    auto lhs = save_proxy(stack, op.instruction.in);
    auto rhs = save_proxy(stack, op.instruction.out);
    std::swap(*lhs.target, *rhs.target);
}

auto execute(ATOM const op, Stack& stack, ip_type const) -> void
{
    auto target = save_proxy(stack, op.instruction.out);

    auto const& strtab     = *stack.proc->strtab;
    auto const data_offset = target.get<uint64_t>();
    if (not data_offset.has_value()) {
        throw abort_execution{stack, "invalid operand for atom constructor"};
    }
    auto const data_size   = [&strtab, data_offset]() -> uint64_t {
        auto const size_offset = (*data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strtab[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto const data_address = reinterpret_cast<char const*>(&strtab[0] + *data_offset);
    auto const key = reinterpret_cast<uint64_t>(data_address);
    auto value = std::string{data_address, data_size};
    stack.proc->atoms[key] = std::move(value);
    target = register_type::atom_type{key};
}
auto execute(STRING const, Stack&, ip_type const) -> void
{
#if 0
    auto target = get_proxy(stack, op.instruction.out, ip);

    auto const& strtab     = *stack.proc->strtab;
    auto const data_offset = cast_to<uint64_t>(target.view());
    auto const data_size   = [&strtab, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strtab[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto s     = std::make_unique<viua::vm::types::String>();
    s->content = std::string{
        reinterpret_cast<char const*>(&strtab[0] + data_offset), data_size};

    target = std::move(s);
#endif
}

auto execute(FRAME const op, Stack& stack, ip_type const) -> void
{
    auto const index = op.instruction.out.index;
    auto const rs    = op.instruction.out.set;

    auto capacity = viua::arch::register_index_type{};
    switch (rs) {
    case viua::arch::RS::LOCAL:
        if (auto v = fetch_proxy(stack, op.instruction.out).get<uint64_t>(); v) {
            capacity = *v;
        } else {
            throw abort_execution{
                stack, "dynamic args count must be an unsigned integer"};
        }
        break;
    case viua::arch::RS::ARGUMENT:
        capacity = index;
        break;
    default:
        throw abort_execution{
            stack, "args count must come from local or argument register set"};
    }

    stack.args = std::vector<register_type>(capacity);
}

auto execute(CALL const op, Stack& stack, ip_type const) -> ip_type
{
    auto fn_name = std::string{};
    auto fn_addr = size_t{};
    {
        if (auto fn = save_proxy(stack, op.instruction.in).get<uint64_t>(); fn) {
            std::tie(fn_name, fn_addr) =
                stack.proc->module.function_at(*fn);
            fn.reset();
        }
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{stack, "invalid IP after call"};
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
    stack.frames.back().saved.fp = stack.proc->frame_pointer;
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
    stack.proc->frame_pointer = stack.proc->stack_break;
    stack.frames.back().saved.fp = stack.proc->frame_pointer;
    stack.frames.back().saved.sbrk = stack.proc->stack_break;

    return fr_entry;
}

auto execute(RETURN const op, Stack& stack, ip_type const) -> ip_type
{
    auto fr = std::move(stack.frames.back());
    stack.frames.pop_back();

    if (auto const rt = fr.result_to; not rt.is_void()) {
        // FIXME detect trying to return a dereference and throw an exception.
        // The following code is invalid and should be rejected:
        //
        //      return *1
        //
        // It would be best if the static analysis phase during assembly caught
        // such errors and refused to produce the ELF output.
        auto const ret = fetch_proxy(fr, op.instruction.out, stack);
        if (ret.holds<void>()) {
            throw abort_execution{
                stack, "return value requested from function returning void"};
        }

        save_proxy(stack, rt) = ret;
    }

    stack.proc->frame_pointer = stack.frames.back().saved.fp;
    stack.proc->stack_break = stack.frames.back().saved.sbrk;
    stack.proc->prune_pointers();

    return fr.return_address;
}

auto execute(LUI const op, Stack& stack, ip_type const) -> void
{
    auto out = save_proxy(stack, op.instruction.out);
    out = static_cast<int64_t>(op.instruction.immediate << 28);
}
auto execute(LUIU const op, Stack& stack, ip_type const) -> void
{
    auto out = save_proxy(stack, op.instruction.out);
    out = static_cast<uint64_t>(op.instruction.immediate << 28);
}

auto execute(FLOAT const op, Stack& stack, ip_type const) -> void
{
    auto target = save_proxy(stack, op.instruction.out);

    auto const& strtab = *stack.proc->strtab;

    auto const data_offset = target.get<uint64_t>();
    if (not data_offset.has_value()) {
        throw abort_execution{stack, "invalid operand"};
    }

    auto const data_size   = [&strtab, data_offset]() -> uint64_t {
        auto const size_offset = (*data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strtab[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint32_t{};
    memcpy(&tmp, (&strtab[0] + *data_offset), data_size);
    tmp = le32toh(tmp);

    auto v = float{};
    memcpy(&v, &tmp, data_size);

    target = v;
}
auto execute(DOUBLE const op, Stack& stack, ip_type const) -> void
{
    auto target = save_proxy(stack, op.instruction.out);

    auto const& strtab = *stack.proc->strtab;

    auto const data_offset = target.get<uint64_t>();
    if (not data_offset.has_value()) {
        throw abort_execution{stack, "invalid operand"};
    }

    auto const data_size   = [&strtab, data_offset]() -> uint64_t {
        auto const size_offset = (*data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strtab[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint64_t{};
    memcpy(&tmp, (&stack.proc->strtab->operator[](0) + *data_offset), data_size);
    tmp = le64toh(tmp);

    auto v = double{};
    memcpy(&v, &tmp, data_size);

    target = v;
}

template<typename Op>
auto execute_arithmetic_immediate_op(Op const op,
                                     Stack& stack) -> void
{
    auto out = save_proxy(stack, op.instruction.out);
    auto in  = fetch_proxy(stack, op.instruction.in);

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
        out = typename Op::functor_type{}(0, immediate);
    } else if (auto const v = in.template get<uint64_t>(); v) {
        out = typename Op::functor_type{}(*v, immediate);
    } else if (auto const v = in.template get<int64_t>(); v) {
        out = typename Op::functor_type{}(*v, immediate);
    } else if (auto const v = in.template get<float>(); v) {
        out = typename Op::functor_type{}(*v, immediate);
    } else if (auto const v = in.template get<double>(); v) {
        out = typename Op::functor_type{}(*v, immediate);
    } else if (auto const v = in.template get<double>(); v) {
        out = typename Op::functor_type{}(*v, immediate);
    } else if (auto const v = in.template get<register_type::pointer_type>(); v and not signed_immediate) {
        auto const r = typename Op::functor_type{}(v->ptr, immediate);
        out = register_type::pointer_type{static_cast<uint64_t>(r)};
    } else {
        throw abort_execution{
            stack,
            "unsupported lhs operand type for immediate arithmetic operation: "
                + std::string{in.type_name()}};
    }
}
auto execute(ADDI const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(ADDIU const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(SUBI const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(SUBIU const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(MULI const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(MULIU const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(DIVI const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}
auto execute(DIVIU const op, Stack& stack, ip_type const) -> void
{
    execute_arithmetic_immediate_op(op, stack);
}

auto execute(REF const, Stack&, ip_type const) -> void
{
#if 0
    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_slot(op.instruction.in, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{stack, "cannot take pointer to void"};
    }

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    if (src.value()->holds<int64_t>()) {
        src.value()->value =
            std::make_unique<Signed_integer>(src.value()->value.get<int64_t>());
    } else if (src.value()->holds<uint64_t>()) {
        src.value()->value = std::make_unique<Unsigned_integer>(
            src.value()->value.get<uint64_t>());
    } else if (src.value()->holds<float>()) {
        src.value()->value =
            std::make_unique<Float_single>(src.value()->value.get<float>());
    } else if (src.value()->holds<double>()) {
        src.value()->value =
            std::make_unique<Float_double>(src.value()->value.get<double>());
    }

    dst.value()->value = src.value()->boxed_value().reference_to();
#endif
}

auto execute(IF const op, Stack& stack, ip_type const ip) -> ip_type
{
    auto const condition = fetch_proxy(stack, op.instruction.out);
    auto tt              = save_proxy(stack, op.instruction.in);

    auto take_branch = (condition.holds<void>() or *condition.cast_to<bool>());
    auto const target =
        take_branch
            ? (stack.back().entry_address + *tt.target->get<uint64_t>())
            : (ip + 1);

    tt.reset();
    return target;
}

auto execute(IO_SUBMIT const, Stack&, ip_type const) -> void
{
#if 0
    auto dst = save_proxy(stack, op.instruction.out, ip);
    auto port = fetch_proxy(stack, op.instruction.lhs, ip);
    auto req = fetch_proxy(stack, op.instruction.rhs, ip);
                   .boxed_of<viua::vm::types::Struct>();

    if (not port.holds<int64_t>()) {
        throw abort_execution{stack, "invalid I/O port"};
    }

    auto& request = req.value().get();
    switch (request.at("opcode").get<int64_t>()) {
    case 0:
    {
        auto buf      = std::move(req.value()
                                 .get()
                                 .at("buf")
                                 .view()
                                 .boxed_of<viua::vm::types::String>()
                                 ->get()
                                 .content);
        auto const rd = stack.proc->core->io.schedule(
            port.get<int64_t>(), IORING_OP_READ, std::move(buf));
        dst = rd;
        break;
    }
    case 1:
    {
        auto buf      = std::move(req.value()
                                 .get()
                                 .at("buf")
                                 .view()
                                 .boxed_of<viua::vm::types::String>()
                                 ->get()
                                 .content);
        auto const rd = stack.proc->core->io.schedule(
            port.get<int64_t>(), IORING_OP_WRITE, std::move(buf));
        dst = rd;
        break;
    }
    }
#endif
}
auto execute(IO_WAIT const, Stack&, ip_type const) -> void
{
#if 0
    auto dst = get_proxy(stack, op.instruction.out, ip);
    auto req = get_value(stack, op.instruction.lhs, ip);

    auto const want_id = req.get<uint64_t>();
    if (not stack.proc->core->io.requests.contains(want_id)) {
        io_uring_cqe* cqe{};
        do {
            io_uring_wait_cqe(&stack.proc->core->io.ring, &cqe);

            if (cqe->res == -1) {
                stack.proc->core->io.requests[cqe->user_data]->status =
                    IO_request::Status::Error;
            } else {
                auto& rd  = *stack.proc->core->io.requests[cqe->user_data];
                rd.status = IO_request::Status::Success;

                if (rd.opcode == IORING_OP_READ) {
                    rd.buffer.resize(cqe->res);
                } else if (rd.opcode == IORING_OP_WRITE) {
                    rd.buffer = rd.buffer.substr(cqe->res);
                }
            }

            io_uring_cqe_seen(&stack.proc->core->io.ring, cqe);
        } while (cqe->user_data != want_id);
    }

    dst = std::make_unique<types::String>(
        std::move(stack.proc->core->io.requests[want_id]->buffer));
    stack.proc->core->io.requests.erase(want_id);
#endif
}
auto execute(IO_SHUTDOWN const, Stack&, ip_type const) -> void
{}
auto execute(IO_CTL const, Stack&, ip_type const) -> void
{}
auto execute(IO_PEEK const, Stack&, ip_type const) -> void
{}

auto execute(ACTOR const op, Stack& stack, ip_type const) -> void
{
    auto fn_name = std::string{};
    auto fn_addr = size_t{};
    {
        if (auto fn = fetch_proxy(stack, op.instruction.in).get<uint64_t>(); fn) {
            std::tie(fn_name, fn_addr) =
                stack.proc->module.function_at(*fn);
            save_proxy(stack, op.instruction.in).reset();
        } else {
            throw abort_execution{stack, "invalid in operand to actor instruction"};
        }
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{stack, "invalid IP after call"};
    }

    auto const fr_entry = (fn_addr / sizeof(viua::arch::instruction_type));

    auto const pid     = stack.proc->core->spawn("", fr_entry);
    auto dst           = save_proxy(stack, op.instruction.out);
    dst = pid.get();
}
auto execute(SELF const op, Stack& stack, ip_type const) -> void
{
    save_proxy(stack, op.instruction.out) = stack.proc->pid.get();
}

auto dump_registers(std::vector<register_type> const& registers,
                    Process::atoms_map_type const& atoms,
                    std::string_view const suffix) -> void
{
    for (auto i = size_t{0}; i < registers.size(); ++i) {
        auto const& each = registers.at(i);
        if (each.is_void()) {
            continue;
        }

        TRACE_STREAM << "      " << std::setw(7) << std::setfill(' ')
                     << ('[' + std::to_string(i) + '.' + suffix.data() + ']')
                     << ' ';

        if (each.is_void()) {
            /* do nothing */
        } else if (auto const v = each.get<register_type::undefined_type>(); v) {
            TRACE_STREAM
                << "raw"
                << std::hex
                << std::setfill('0');
            for (auto const each : *v) {
                TRACE_STREAM << " " << std::setw(2) << static_cast<unsigned>(each);
            }
            TRACE_STREAM << '\n';
        } else if (auto const v = each.get<int64_t>(); v) {
            TRACE_STREAM << "is " << std::hex << std::setw(16)
                         << std::setfill('0') << *v
                         << " " << std::dec << *v
                         << '\n';
        } else if (auto const v = each.get<uint64_t>(); v) {
            TRACE_STREAM << "iu " << std::hex << std::setw(16)
                         << std::setfill('0') << *v
                         << " " << std::dec << *v
                         << '\n';
        } else if (auto const v = each.get<float>(); v) {
            auto const precision = std::cerr.precision();
            TRACE_STREAM
                << "fl " << std::hexfloat << *v << " "
                << std::defaultfloat
                << std::setprecision(std::numeric_limits<float>::digits10 + 1)
                << *v << '\n';
            TRACE_STREAM << std::setprecision(precision);
        } else if (auto const v = each.get<double>(); v) {
            auto const precision = std::cerr.precision();
            TRACE_STREAM
                << "db " << std::hexfloat << *v << " "
                << std::defaultfloat
                << std::setprecision(std::numeric_limits<double>::digits10 + 1)
                << *v << '\n';
            TRACE_STREAM << std::setprecision(precision);
        } else if (auto const v = each.get<register_type::pointer_type>(); v) {
            TRACE_STREAM << "ptr " << std::hex << std::setw(16)
                         << std::setfill('0') << v->ptr
                         << " " << std::dec << v->ptr
                         << '\n';
        } else if (auto const v = each.get<register_type::atom_type>(); v) {
            TRACE_STREAM << "atom " << atoms.at(v->key) << '\n';
        } else if (auto const v = each.get<register_type::pid_type>(); v) {
            TRACE_STREAM << "pid " << viua::runtime::PID{*v}.to_string() << '\n';
        }
    }
}
auto print_backtrace_line(
    Stack const& stack,
    size_t const frame_index,
    std::map<Frame::addr_type, std::string> const& fn_entry_to_name) -> void
{
    auto const& each = stack.frames.at(frame_index);
    viua::TRACE_STREAM << "    #" << frame_index << "  ";

    if (fn_entry_to_name.count(each.entry_address)) {
        viua::TRACE_STREAM << fn_entry_to_name.at(each.entry_address);
    } else {
        viua::TRACE_STREAM << "??";
    }
    viua::TRACE_STREAM << (each.parameters.empty() ? " ()" : " (...)")
                       << " at ";

    viua::TRACE_STREAM << stack.proc->module.elf_path.native() << "[.text+0x"
                       << std::hex << std::setw(8) << std::setfill('0');

    auto ip_offset = size_t{};
    if (frame_index < (stack.frames.size() - 1)) {
        ip_offset = (stack.frames.at(frame_index + 1).return_address
                     - stack.proc->module.ip_base);
    } else {
        ip_offset = (stack.ip - stack.proc->module.ip_base);
    }
    viua::TRACE_STREAM << (ip_offset * sizeof(viua::arch::instruction_type));
    viua::TRACE_STREAM << std::dec << ']';

    viua::TRACE_STREAM << " return to ";
    if (each.return_address) {
        viua::TRACE_STREAM << stack.proc->module.elf_path.native() << "[.text+0x"
                           << std::hex << std::setw(8) << std::setfill('0')
                           << ((each.return_address - stack.proc->module.ip_base)
                               * sizeof(viua::arch::instruction_type))
                           << std::dec << ']';
    } else {
        viua::TRACE_STREAM << "null";
    }

    viua::TRACE_STREAM << viua::TRACE_STREAM.endl;
}
auto print_backtrace(Stack const& stack, std::optional<size_t> const only_for)
    -> void
{
    auto fn_entry_to_name = std::map<Frame::addr_type, std::string>{};
    {
        for (auto const& [fn_off, fn] :
             stack.proc->module.elf.function_table()) {
            auto const [fn_name, fn_entry] = fn;
            auto const entry_addr =
                (stack.proc->module.ip_base
                 + (fn_entry / sizeof(viua::arch::instruction_type)));
            fn_entry_to_name.emplace(entry_addr, fn_name);
        }
    }

    if (only_for.has_value()) {
        print_backtrace_line(stack, *only_for, fn_entry_to_name);
    } else {
        for (auto i = size_t{0}; i < stack.frames.size(); ++i) {
            print_backtrace_line(stack, i, fn_entry_to_name);
        }
    }
}
auto dump_memory(std::vector<Page> const& memory) -> void
{
    viua::TRACE_STREAM << "  memory:" << viua::TRACE_STREAM.endl;

    viua::TRACE_STREAM << std::hex << std::setfill('0');
    for (auto line = size_t{0}; line < (memory.front().size() / MEM_LINE_SIZE); ++line) {
        viua::TRACE_STREAM << "    ";
        viua::TRACE_STREAM << std::setw(16) << (line * MEM_LINE_SIZE) << "  ";
        for (auto i = size_t{0}; i < MEM_LINE_SIZE; ++i) {
            viua::TRACE_STREAM
                << std::setw(2)
                << static_cast<int>(memory.front().at(line * MEM_LINE_SIZE + i))
                << ' ';
        }
        viua::TRACE_STREAM << " | ";
        for (auto i = size_t{0}; i < MEM_LINE_SIZE; ++i) {
            auto const c = memory.front().at(line * MEM_LINE_SIZE + i);
            viua::TRACE_STREAM << (isprint(c) ? static_cast<char>(c) : '.');
        }
        viua::TRACE_STREAM << viua::TRACE_STREAM.endl;
    }
}
auto execute(EBREAK const, Stack& stack, ip_type const) -> void
{
    viua::TRACE_STREAM << "begin ebreak in process "
                       << stack.proc->pid.to_string() << viua::TRACE_STREAM.endl;

    viua::TRACE_STREAM << "  backtrace:" << viua::TRACE_STREAM.endl;
    print_backtrace(stack);

    viua::TRACE_STREAM << "  register contents:" << viua::TRACE_STREAM.endl;
    for (auto i = size_t{0}; i < stack.frames.size(); ++i) {
        auto const& each = stack.frames.at(i);

        viua::TRACE_STREAM << "    of #" << i << viua::TRACE_STREAM.endl;

        auto const fptr = each.saved.fp;
        auto const sbrk = each.saved.sbrk;
        TRACE_STREAM << "        [fptr] "
            << "iu " << std::hex << std::setw(16)
                     << std::setfill('0') << fptr
                     << " " << std::dec << fptr
                     << '\n';
        TRACE_STREAM << "        [sbrk] "
            << "iu " << std::hex << std::setw(16)
                     << std::setfill('0') << sbrk
                     << " " << std::dec << sbrk
                     << '\n';

        dump_registers(each.parameters, stack.proc->atoms, "p");
        dump_registers(each.registers, stack.proc->atoms, "l");
    }
    dump_registers(stack.args, stack.proc->atoms, "a");

    dump_memory(stack.proc->memory);

    viua::TRACE_STREAM << "end ebreak in process " << stack.proc->pid.to_string()
                       << viua::TRACE_STREAM.endl;
}
auto execute(ECALL const, Stack&, ip_type const) -> void
{
}

auto execute(SM const op, Stack& stack, ip_type const) -> void
{
    auto const base       = fetch_proxy(stack, op.instruction.in).get<register_type::pointer_type>();

    auto const unit       = op.instruction.spec;
    auto const copy_size  = (1u << unit);
    auto const offset     = (op.instruction.immediate * copy_size);

    if (not base.has_value()) {
        throw abort_execution{stack, "invalid base operand for memory instruction"};
    }

    auto const pointer_info = stack.proc->get_pointer(base->ptr);
    if (not pointer_info.has_value()) {
        auto o = std::ostringstream{};
        o << "unknown pointer: ";
        o << std::hex << std::setfill('0') << std::setw(16) << base->ptr;
        throw abort_execution{stack, o.str()};
    }

    if (offset >= pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal offset of " << offset
            << " bytes into a region of "
            << pointer_info->size << " byte(s)";
        throw abort_execution{stack, o.str()};
    }
    if ((offset + copy_size) > pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal store of " << copy_size
            << " bytes into a region of "
            << (pointer_info->size - offset) << " byte(s)";
        throw abort_execution{stack, o.str()};
    }

    auto const user_addr = base->ptr + offset;
    auto const addr = stack.proc->memory_at(user_addr);
    if (addr == nullptr) {
        auto o = std::ostringstream{};
        o << "invalid store address: ";
        o << std::hex << std::setfill('0') << std::setw(16) << user_addr;
        throw abort_execution{stack, o.str()};
    }

    auto const in = fetch_proxy(stack, op.instruction.out);
    if (in.holds<void>()) {
        throw abort_execution{stack, "invalid in operand for memory instruction"};
    }
    auto const buf = in.target.as_memory();
    memcpy(addr, buf.data(), copy_size);
}
auto execute(LM const op, Stack& stack, ip_type const) -> void
{
    auto const base       = fetch_proxy(stack, op.instruction.in).get<register_type::pointer_type>();

    auto const unit       = op.instruction.spec;
    auto const copy_size  = (1u << unit);
    auto const offset     = (op.instruction.immediate * copy_size);

    if (not base.has_value()) {
        throw abort_execution{stack, "invalid base operand for memory instruction"};
    }

    auto const pointer_info = stack.proc->get_pointer(base->ptr);
    if (not pointer_info.has_value()) {
        auto o = std::ostringstream{};
        o << "unknown pointer: ";
        o << std::hex << std::setfill('0') << std::setw(16) << base->ptr;
        throw abort_execution{stack, o.str()};
    }

    if (offset >= pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal offset of " << offset
            << " bytes into a region of "
            << pointer_info->size << " byte(s)";
        throw abort_execution{stack, o.str()};
    }
    if ((offset + copy_size) > pointer_info->size) {
        auto o = std::ostringstream{};
        o << "illegal load of " << copy_size
            << " bytes from a region of "
            << (pointer_info->size - offset) << " byte(s)";
        throw abort_execution{stack, o.str()};
    }

    auto const user_addr = base->ptr + offset;
    auto const addr = stack.proc->memory_at(user_addr);
    if (addr == nullptr) {
        auto o = std::ostringstream{};
        o << "invalid load address: ";
        o << std::hex << std::setfill('0') << std::setw(16) << user_addr;
        throw abort_execution{stack, o.str()};
    }

    auto out = save_proxy(stack, op.instruction.out);
    auto raw = register_type::undefined_type{};
    memcpy(raw.data(), addr, copy_size);
    out = raw;
}
auto execute(AD const, Stack&, ip_type const) -> void
{
}
auto execute(PTR const, Stack&, ip_type const) -> void
{
}
}  // namespace viua::vm::ins
