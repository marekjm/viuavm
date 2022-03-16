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
using ip_type = viua::arch::instruction_type const*;

namespace {
auto type_name(viua::vm::types::Cell_view const c) -> std::string
{
    using viua::vm::types::Cell_view;

    if (std::holds_alternative<std::monostate>(c.content)) {
        return "void";
    } else if (std::holds_alternative<std::reference_wrapper<int64_t>>(
                   c.content)) {
        return "int";
    } else if (std::holds_alternative<std::reference_wrapper<uint64_t>>(
                   c.content)) {
        return "uint";
    } else if (std::holds_alternative<std::reference_wrapper<float>>(
                   c.content)) {
        return "float";
    } else if (std::holds_alternative<std::reference_wrapper<double>>(
                   c.content)) {
        return "double";
    } else {
        return std::get<std::reference_wrapper<Cell_view::boxed_type>>(
                   c.content)
            .get()
            .type_name();
    }
}
auto get_slot(viua::arch::Register_access const ra,
              Stack& stack,
              ip_type const ip) -> std::optional<viua::vm::Value*>
{
    switch (ra.set) {
        using enum viua::arch::RS;
    case VOID:
        return {};
    case LOCAL:
        return &stack.frames.back().registers.at(ra.index);
    case ARGUMENT:
        return &stack.args.at(ra.index);
    case PARAMETER:
        return &stack.frames.back().parameters.at(ra.index);
    }

    throw abort_execution{ip, "impossible"};
}

struct Proxy {
    using slot_type = std::reference_wrapper<viua::vm::Value>;
    using cell_type = viua::vm::types::Cell_view;

    std::variant<slot_type, cell_type> slot;

    explicit Proxy(slot_type s) : slot{s}
    {}
    explicit Proxy(cell_type c) : slot{c}
    {}

    auto hard() const -> bool
    {
        return std::holds_alternative<slot_type>(slot);
    }
    auto soft() const -> bool
    {
        return not hard();
    }

    auto view() const -> cell_type const
    {
        if (hard()) {
            return std::get<slot_type>(slot).get().value.view();
        } else {
            return std::get<cell_type>(slot);
        }
    }
    auto view() -> cell_type
    {
        if (hard()) {
            return std::get<slot_type>(slot).get().value.view();
        } else {
            return std::get<cell_type>(slot);
        }
    }
    auto overwrite() -> slot_type::type&
    {
        return std::get<slot_type>(slot).get();
    }

    template<typename T> auto operator=(T&& v) -> Proxy&
    {
        overwrite() = std::move(v);
        return *this;
    }

    template<typename T>
    inline auto boxed_of() -> std::optional<std::reference_wrapper<T>>
    {
        return view().boxed_of<T>();
    }
};
auto get_proxy(std::vector<viua::vm::Value>& registers,
               viua::arch::Register_access const a,
               ip_type const ip) -> Proxy
{
    auto& c = registers.at(a.index);
    if (a.direct) {
        return Proxy{c};
    }

    using viua::vm::types::Cell;
    using viua::vm::types::Cell_view;

    if (not std::holds_alternative<Cell::boxed_type>(c.value.content)) {
        throw abort_execution{
            ip, "cannot dereference a value of type " + type_name(c.value)};
    }

    auto& boxed = std::get<Cell::boxed_type>(c.value.content);
    if (auto p = dynamic_cast<types::Ref*>(boxed.get()); p) {
        return Proxy{Cell_view{*p->value}};
    }

    throw abort_execution{
        ip, "cannot dereference a value of type " + type_name(c.value)};
}
auto get_proxy(Stack& stack,
               viua::arch::Register_access const a,
               ip_type const ip) -> Proxy
{
    switch (a.set) {
        using enum viua::arch::Register_access::set_type;
    case VOID:
        throw abort_execution{ip, "cannot access a void register"};
    case LOCAL:
        return get_proxy(stack.frames.back().registers, a, ip);
    case ARGUMENT:
        std::cerr << "getting proxy for argument register\n";
        return get_proxy(stack.args, a, ip);
    case PARAMETER:
        return get_proxy(stack.frames.back().parameters, a, ip);
    default:
        throw abort_execution{ip, "access to invalid register set"};
    }
}

auto type_name(Proxy const& p) -> std::string
{
    return type_name(p.view());
}

auto get_value(std::vector<viua::vm::Value>& registers,
               viua::arch::Register_access const a,
               ip_type const ip) -> viua::vm::types::Cell_view
{
    using viua::vm::types::Cell;
    using viua::vm::types::Cell_view;

    auto& c = registers.at(a.index);
    if (a.direct) {
        return c.value.view();
    }

    if (not std::holds_alternative<Cell::boxed_type>(c.value.content)) {
        throw abort_execution{
            ip, "cannot dereference a value of type " + type_name(c.value)};
    }

    auto& boxed = std::get<Cell::boxed_type>(c.value.content);
    if (auto p = dynamic_cast<types::Ref*>(boxed.get()); p) {
        return Cell_view{*p->value};
    }

    throw abort_execution{
        ip, "cannot dereference a value of type " + type_name(c.value)};
}
auto get_value(Stack& stack,
               viua::arch::Register_access const a,
               ip_type const ip) -> viua::vm::types::Cell_view
{
    return get_value(stack.frames.back().registers, a, ip);
}
}  // namespace

template<typename T> auto cast_to(viua::vm::types::Cell_view value) -> T
{
    using viua::vm::types::Cell_view;

    using Tr = std::reference_wrapper<T>;

    if (std::holds_alternative<Tr>(value.content)) {
        return value.template get<T>();
    }

    if (value.template holds<int64_t>()) {
        return static_cast<T>(value.template get<int64_t>());
    }
    if (value.template holds<uint64_t>()) {
        return static_cast<T>(value.template get<uint64_t>());
    }
    if (value.template holds<float>()) {
        return static_cast<T>(value.template get<float>());
    }
    if (value.template holds<double>()) {
        return static_cast<T>(value.template get<double>());
    }

    if (not value.template holds<Cell_view::boxed_type>()) {
        throw std::bad_cast{};
    }

    auto const bv = &value.template get<Cell_view::boxed_type>();

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    if (auto x = dynamic_cast<Signed_integer const*>(bv); x) {
        return static_cast<T>(x->value);
    }
    if (auto x = dynamic_cast<Unsigned_integer const*>(bv); x) {
        return static_cast<T>(x->value);
    }
    if (auto x = dynamic_cast<Float_single const*>(bv); x) {
        return static_cast<T>(x->value);
    }
    if (auto x = dynamic_cast<Float_double const*>(bv); x) {
        return static_cast<T>(x->value);
    }

    throw std::bad_cast{};
}

template<typename Boxed, typename T>
auto store_impl(ip_type const ip,
                Proxy& out,
                T value,
                std::string_view const tn) -> void
{
    /*
     * This is the simple case. We got an output operand which was specified as
     * a direct register access ie, a "hard" access. This means we can just
     * blindly assign the result to the proxy and perform a destructive store.
     *
     * Example code:
     *
     *      li $1, 23
     *      li $2, 19
     *      add $2, $1, $2
     *
     * The value 19 in local register 2 will be destroyed and overwritten with a
     * new one - a 42. It does not matter that they are both signed 64-bit wide
     * integers. A direct store is a destructive store. Always.
     *
     * Keep in mind that this will invalidate any references that were pointing
     * to the old value, since the old value will be destroyed to make space for
     * the new one. A static analyser should catch this situations and reject
     * any code which contains a use of such dangling references.
     */
    if (out.hard()) {
        out = std::move(value);
        return;
    }

    /*
     * If the output operand is not a hard access which allows destructive
     * stores then it must be soft access ie, through a reference, which allows
     * mutation.
     *
     * If that is the case then the register selected by the output operand MUST
     * contain a value of the same type as the result value produced by the
     * operation. Otherwise, the operation is illegal because it is not
     * reasonable to mutate, for example, a string using an unsigned integer or
     * vice versa.
     */
    if (not out.view().template holds<Boxed>()) {
        throw abort_execution{ip,
                              (std::string{"cannot mutate "} + tn.data()
                               + " through a reference to " + type_name(out))};
    }

    /*
     * After ruling out hard and illegal accesses, the only case left is soft
     * access. This means that we have a dereference as the output operand and
     * the program wants to mutate a value instead of performing a destructive
     * store.
     */
    auto b                = out.view().template boxed_of<Boxed>();
    b.value().get().value = std::move(value);
}

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
template<typename Op, typename Trait>
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

auto execute(ADD const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_op<ADD, viua::vm::types::traits::Plus>(op, stack, ip);
}
auto execute(SUB const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_op(op, stack, ip);
}
auto execute(MUL const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_op(op, stack, ip);
}
auto execute(DIV const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_op(op, stack, ip);
}
auto execute(MOD const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;

    auto const holds_i64 =
        (lhs.template holds<int64_t>() or lhs.template holds<Signed_integer>());
    auto const holds_u64 = (lhs.template holds<uint64_t>()
                            or lhs.template holds<Unsigned_integer>());

    if (holds_i64) {
        store_impl<Signed_integer>(
            ip, out, (cast_to<int64_t>(lhs) % cast_to<int64_t>(rhs)), "i64");
    } else if (holds_u64) {
        store_impl<Unsigned_integer>(
            ip, out, (cast_to<uint64_t>(lhs) % cast_to<uint64_t>(rhs)), "u64");
    } else {
        throw abort_execution{ip,
                              "unsupported operand types for modulo operation"};
    }
}

auto execute(BITSHL const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Unsigned_integer;
    store_impl<Unsigned_integer>(
        ip, out, (lhs.get<uint64_t>() << rhs.get<uint64_t>()), "u64");
}
auto execute(BITSHR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Unsigned_integer;
    store_impl<Unsigned_integer>(
        ip, out, (lhs.get<uint64_t>() >> rhs.get<uint64_t>()), "u64");
}
auto execute(BITASHR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Unsigned_integer;
    auto const tmp = static_cast<int64_t>(lhs.get<uint64_t>());
    store_impl<Unsigned_integer>(
        ip, out, static_cast<uint64_t>(tmp >> rhs.get<uint64_t>()), "u64");
}
auto execute(BITROL const, Stack&, ip_type const) -> void
{}
auto execute(BITROR const, Stack&, ip_type const) -> void
{}
auto execute(BITAND const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Unsigned_integer;
    store_impl<Unsigned_integer>(
        ip, out, (lhs.get<uint64_t>() & rhs.get<uint64_t>()), "u64");
}
auto execute(BITOR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Unsigned_integer;
    store_impl<Unsigned_integer>(
        ip, out, (lhs.get<uint64_t>() | rhs.get<uint64_t>()), "u64");
}
auto execute(BITXOR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::Unsigned_integer;
    store_impl<Unsigned_integer>(
        ip, out, (lhs.get<uint64_t>() ^ rhs.get<uint64_t>()), "u64");
}
auto execute(BITNOT const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_proxy(stack, op.instruction.out, ip);
    auto in  = get_value(stack, op.instruction.in, ip);

    using viua::vm::types::Unsigned_integer;
    store_impl<Unsigned_integer>(ip, out, ~in.get<uint64_t>(), "u64");
}

auto execute(EQ const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    using viua::vm::types::traits::Eq;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Atom;
    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::String;
    using viua::vm::types::Unsigned_integer;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Eq>()) {
        auto const& cmp = lhs.boxed_of<Eq>().value().get();
        cmp_result      = cmp(cmp, rhs);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result      = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for eq"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot eq unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out      = (cmp_result == 0);
}
auto execute(LT const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Atom;
    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::String;
    using viua::vm::types::Unsigned_integer;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result      = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for lt"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot lt unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out      = (cmp_result < 0);
}
auto execute(GT const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Atom;
    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::String;
    using viua::vm::types::Unsigned_integer;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result      = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for gt"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot gt unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out      = (cmp_result > 0);
}
auto execute(CMP const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Atom;
    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::String;
    using viua::vm::types::Unsigned_integer;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result   = (l <=> r);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result      = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for cmp"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot cmp unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out      = (cmp_result < 0) ? -1 : (0 < cmp_result) ? 1 : 0;
}
auto execute(AND const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    using viua::vm::types::traits::Bool;
    if (lhs.is_boxed() and not lhs.boxed_value().has_trait<Bool>()) {
        throw abort_execution{
            nullptr, "and: cannot used boxed value without Bool trait"};
    }

    if (lhs.is_boxed()) {
        auto const use_lhs = not lhs.boxed_value().as_trait<Bool, bool>(
            [](Bool const& v) -> bool { return static_cast<bool>(v); }, false);
        out = use_lhs ? std::move(lhs) : std::move(rhs);
    } else {
        auto const use_lhs = (lhs.value.get<uint64_t>() == 0);
        out                = use_lhs ? std::move(lhs) : std::move(rhs);
    }
}
auto execute(OR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    using viua::vm::types::traits::Bool;
    if (lhs.is_boxed() and not lhs.boxed_value().has_trait<Bool>()) {
        throw abort_execution{nullptr,
                              "or: cannot used boxed value without Bool trait"};
    }

    if (lhs.is_boxed()) {
        auto const use_lhs = lhs.boxed_value().as_trait<Bool, bool>(
            [](Bool const& v) -> bool { return static_cast<bool>(v); }, false);
        out = use_lhs ? std::move(lhs) : std::move(rhs);
    } else {
        auto const use_lhs = (lhs.value.get<uint64_t>() != 0);
        out                = use_lhs ? std::move(lhs) : std::move(rhs);
    }
}
auto execute(NOT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& in        = registers.at(op.instruction.in.index);

    using viua::vm::types::traits::Bool;
    if (in.is_boxed() and not in.boxed_value().has_trait<Bool>()) {
        throw abort_execution{
            nullptr, "not: cannot used boxed value without Bool trait"};
    }

    if (in.is_boxed()) {
        out = in.boxed_value().as_trait<Bool, bool>(
            [](Bool const& v) -> bool { return static_cast<bool>(v); }, false);
    } else {
        out = static_cast<bool>(in.value.get<uint64_t>());
    }
}

auto execute(COPY const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& in        = registers.at(op.instruction.in.index);
    auto& out       = registers.at(op.instruction.out.index);

    using viua::vm::types::traits::Copy;
    if (in.is_boxed() and not in.boxed_value().has_trait<Copy>()) {
        throw abort_execution{nullptr,
                              "value of type " + in.boxed_value().type_name()
                                  + " is not copyable"};
    }

    if (in.holds<int64_t>()) {
        out = in.value.get<int64_t>();
    } else if (in.holds<uint64_t>()) {
        out = in.value.get<uint64_t>();
    } else if (in.holds<float>()) {
        out = in.value.get<float>();
    } else if (in.holds<double>()) {
        out = in.value.get<double>();
    } else {
        out.value = in.boxed_value().as_trait<Copy>().copy();
    }
}
auto execute(MOVE const op, Stack& stack, ip_type const ip) -> void
{
    auto in = get_proxy(stack, op.instruction.in, ip);

    if (in.view().is_void()) {
        throw abort_execution{ip, "cannot move out of void"};
    }
    if (op.instruction.out.set != viua::arch::Register_access::set_type::VOID) {
        auto out        = get_proxy(stack, op.instruction.out, ip);
        out.overwrite() = std::move(in.overwrite());
    }
    in.overwrite().make_void();
}
auto execute(SWAP const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& lhs       = registers.at(op.instruction.in.index);
    auto& rhs       = registers.at(op.instruction.out.index);

    std::swap(lhs.value, rhs.value);
}

auto execute(ATOM const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto const& mod        = stack.proc.module;
    auto const data_offset = target.value.get<uint64_t>();
    auto const data_size   = [&mod, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &mod.strings_table[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto s     = std::make_unique<viua::vm::types::Atom>();
    s->content = std::string{
        reinterpret_cast<char const*>(&mod.strings_table[0] + data_offset),
        data_size};

    target.value = std::move(s);
}
auto execute(STRING const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto const& mod        = stack.proc.module;
    auto const data_offset = target.value.get<uint64_t>();
    auto const data_size   = [&mod, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &mod.strings_table[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto s     = std::make_unique<viua::vm::types::String>();
    s->content = std::string{
        reinterpret_cast<char const*>(&mod.strings_table[0] + data_offset),
        data_size};

    target.value = std::move(s);
}

auto execute(FRAME const op, Stack& stack, ip_type const ip) -> void
{
    auto const index = op.instruction.out.index;
    auto const rs    = op.instruction.out.set;

    auto capacity = viua::arch::register_index_type{};
    if (rs == viua::arch::RS::LOCAL) {
        capacity = static_cast<viua::arch::register_index_type>(
            stack.back().registers.at(index).value.get<uint64_t>());
    } else if (rs == viua::arch::RS::ARGUMENT) {
        capacity = index;
    } else {
        throw abort_execution{
            ip, "args count must come from local or argument register set"};
    }

    stack.args = std::vector<Value>(capacity);
}

auto execute(CALL const op, Stack& stack, ip_type const ip) -> ip_type
{
    auto fn_name = std::string{};
    auto fn_addr = size_t{};
    {
        auto const fn_index   = op.instruction.in.index;
        auto const& fn_offset = stack.frames.back().registers.at(fn_index);
        if (fn_offset.is_void()) {
            throw abort_execution{ip, "fn offset cannot be void"};
        }
        if (fn_offset.is_boxed()) {
            // FIXME only unboxed integers allowed for now
            throw abort_execution{ip, "fn offset cannot be boxed"};
        }

        std::tie(fn_name, fn_addr) =
            stack.proc.module.function_at(fn_offset.value.get<uint64_t>());

        get_proxy(stack, op.instruction.in, ip).overwrite().make_void();
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{ip, "invalid IP after call"};
    }

    auto const fr_return = (ip + 1);
    auto const fr_entry  = (stack.proc.module.ip_base
                           + (fn_addr / sizeof(viua::arch::instruction_type)));

    stack.frames.emplace_back(
        viua::arch::MAX_REGISTER_INDEX, fr_entry, fr_return);
    stack.frames.back().parameters = std::move(stack.args);
    stack.frames.back().result_to  = op.instruction.out;

    return fr_entry;
}

auto execute(RETURN const op, Stack& stack, ip_type const ip) -> ip_type
{
    auto fr = std::move(stack.frames.back());
    stack.frames.pop_back();

    if (auto const& rt = fr.result_to; not rt.is_void()) {
        if (op.instruction.out.is_void()) {
            throw abort_execution{
                ip, "return value requested from function returning void"};
        }
        auto const index = op.instruction.out.index;
        stack.frames.back().registers.at(rt.index) =
            std::move(fr.registers.at(index));
    }

    return fr.return_address;
}

auto execute(LUI const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& value     = registers.at(op.instruction.out.index);
    value.value     = static_cast<int64_t>(op.instruction.immediate << 28);
}
auto execute(LUIU const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& value     = registers.at(op.instruction.out.index);
    value.value     = (op.instruction.immediate << 28);
}

auto execute(FLOAT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;

    auto& target = registers.at(op.instruction.out.index);

    auto const data_offset = target.value.get<uint64_t>();
    auto const data_size   = [&stack, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp,
               &stack.proc.module.strings_table[size_offset],
               sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint32_t{};
    memcpy(
        &tmp, (&stack.proc.module.strings_table[0] + data_offset), data_size);
    tmp = le32toh(tmp);

    auto v = float{};
    memcpy(&v, &tmp, data_size);

    target.value = v;
}
auto execute(DOUBLE const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;

    auto& target = registers.at(op.instruction.out.index);

    auto const data_offset = target.value.get<uint64_t>();
    auto const data_size   = [&stack, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp,
               &stack.proc.module.strings_table[size_offset],
               sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint64_t{};
    memcpy(
        &tmp, (&stack.proc.module.strings_table[0] + data_offset), data_size);
    tmp = le64toh(tmp);

    auto v = double{};
    memcpy(&v, &tmp, data_size);

    target.value = v;
}
auto execute(STRUCT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::Struct>();
    target.value = std::move(s);
}
auto execute(BUFFER const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::Buffer>();
    target.value = std::move(s);
}

template<typename Op>
auto execute_arithmetic_immediate_op(Op const op,
                                     Stack& stack,
                                     ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto out        = get_proxy(registers, op.instruction.out, ip);
    auto in         = get_value(registers, op.instruction.in, ip);

    constexpr auto const signed_immediate =
        std::is_signed_v<typename Op::value_type>;
    using immediate_type =
        std::conditional<signed_immediate, int64_t, uint64_t>::type;
    auto const immediate =
        (signed_immediate
             ? static_cast<immediate_type>(
                 static_cast<int32_t>(op.instruction.immediate << 8) >> 8)
             : static_cast<immediate_type>(op.instruction.immediate));

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    if (in.template holds<void>()) {
        out = typename Op::functor_type{}(0, immediate);
    } else if (in.template holds<uint64_t>()) {
        out =
            typename Op::functor_type{}(in.template get<uint64_t>(), immediate);
    } else if (in.template holds<int64_t>()) {
        out =
            typename Op::functor_type{}(in.template get<int64_t>(), immediate);
    } else if (in.template holds<float>()) {
        out = typename Op::functor_type{}(in.template get<float>(), immediate);
    } else if (in.template holds<double>()) {
        out = typename Op::functor_type{}(in.template get<double>(), immediate);
    } else if (in.template holds<Signed_integer>()) {
        if (auto b = out.template boxed_of<Signed_integer>(); b.has_value()) {
            auto& boxed = b.value().get();
            boxed.value =
                typename Op::functor_type{}(cast_to<int64_t>(in), immediate);
        } else {
            out = typename Op::functor_type{}(cast_to<int64_t>(in), immediate);
        }
    } else if (in.template holds<Unsigned_integer>()) {
        if (auto b = out.template boxed_of<Unsigned_integer>(); b.has_value()) {
            auto& boxed = b.value().get();
            boxed.value =
                typename Op::functor_type{}(cast_to<uint64_t>(in), immediate);
        } else {
            out = typename Op::functor_type{}(cast_to<uint64_t>(in), immediate);
        }
    } else if (in.template holds<Float_single>()) {
        if (auto b = out.template boxed_of<Float_single>(); b.has_value()) {
            auto& boxed = b.value().get();
            boxed.value =
                typename Op::functor_type{}(cast_to<float>(in), immediate);
        } else {
            out = typename Op::functor_type{}(cast_to<float>(in), immediate);
        }
    } else if (in.template holds<Float_double>()) {
        if (auto b = out.template boxed_of<Float_double>(); b.has_value()) {
            auto& boxed = b.value().get();
            boxed.value =
                typename Op::functor_type{}(cast_to<double>(in), immediate);
        } else {
            out = typename Op::functor_type{}(cast_to<double>(in), immediate);
        }
    } else {
        throw abort_execution{
            ip,
            "unsupported lhs operand type for immediate arithmetic operation: "
                + type_name(in)};
    }
}
auto execute(ADDI const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(ADDIU const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(SUBI const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(SUBIU const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(MULI const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(MULIU const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(DIVI const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}
auto execute(DIVIU const op, Stack& stack, ip_type const ip) -> void
{
    execute_arithmetic_immediate_op(op, stack, ip);
}

auto execute(BUFFER_PUSH const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_value(stack, op.instruction.out, ip);
    auto src = get_proxy(stack, op.instruction.in, ip);

    if (src.view().is_void()) {
        throw abort_execution{ip, "cannot buffer_push out of void"};
    }

    if (not dst.holds<viua::vm::types::Buffer>()) {
        throw abort_execution{ip,
                              "invalid destination operand for buffer_push"};
    }
    auto& b = dst.boxed_of<viua::vm::types::Buffer>().value().get();
    b.push(std::move(src.overwrite().value_cell()));
}
auto execute(BUFFER_SIZE const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_proxy(stack, op.instruction.out, ip);
    auto src = get_proxy(stack, op.instruction.in, ip);

    if (src.view().is_void()) {
        throw abort_execution{ip, "cannot take buffer_size of void"};
    }
    auto const& b =
        src.view().boxed_of<viua::vm::types::Buffer>().value().get();
    dst.overwrite() = b.size();
}
auto execute(BUFFER_AT const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_value(stack, op.instruction.lhs, ip);
    auto idx = get_value(stack, op.instruction.rhs, ip);

    if (src.is_void()) {
        throw abort_execution{ip, "cannot buffer_at out of void"};
    }

    auto& buf = src.boxed_of<viua::vm::types::Buffer>().value().get();
    auto off  = (buf.size() - 1);
    if (idx.is_void()) {
        // do nothing, and use the default value
    } else if (idx.holds<uint64_t>()) {
        off = idx.get<uint64_t>();
    } else if (idx.holds<int64_t>()) {
        auto const i = idx.get<int64_t>();
        if (i < 0) {
            off = (buf.size() + i);
        } else {
            off = i;
        }
    } else {
        throw abort_execution{ip, "buffer index must be an integer"};
    }

    if (buf.size() <= off) {
        throw abort_execution{ip,
                              ("index " + std::to_string(off) + " out of range "
                               + std::to_string(buf.size()))};
    }

    auto& v = buf.at(off);

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    if (std::holds_alternative<int64_t>(v.content)) {
        v.content = std::make_unique<Signed_integer>(v.get<int64_t>());
    } else if (std::holds_alternative<uint64_t>(v.content)) {
        v.content = std::make_unique<Unsigned_integer>(v.get<uint64_t>());
    } else if (std::holds_alternative<float>(v.content)) {
        v.content = std::make_unique<Float_single>(v.get<float>());
    } else if (std::holds_alternative<double>(v.content)) {
        v.content = std::make_unique<Float_double>(v.get<double>());
    }

    using viua::vm::types::Cell;
    *dst.value() = v.get<Cell::boxed_type>()->reference_to();
}
auto execute(BUFFER_POP const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_value(stack, op.instruction.lhs, ip);
    auto idx = get_value(stack, op.instruction.rhs, ip);

    if (src.is_void()) {
        throw abort_execution{ip, "cannot buffer_pop out of void"};
    }

    auto& buf = src.boxed_of<viua::vm::types::Buffer>().value().get();
    auto off  = (buf.size() - 1);
    if (idx.is_void()) {
        // do nothing, and use the default value
    } else if (idx.holds<uint64_t>()) {
        off = idx.get<uint64_t>();
    } else if (idx.holds<int64_t>()) {
        auto const i = idx.get<int64_t>();
        if (i < 0) {
            off = (buf.size() + i);
        } else {
            off = i;
        }
    } else {
        throw abort_execution{ip, "buffer index must be an integer"};
    }

    using viua::vm::types::Cell;
    auto v = buf.pop(off);

    if (std::holds_alternative<int64_t>(v.content)) {
        *dst.value() = v.get<int64_t>();
    } else if (std::holds_alternative<uint64_t>(v.content)) {
        *dst.value() = v.get<uint64_t>();
    } else if (std::holds_alternative<float>(v.content)) {
        *dst.value() = v.get<float>();
    } else if (std::holds_alternative<double>(v.content)) {
        *dst.value() = v.get<double>();
    } else if (std::holds_alternative<Cell::boxed_type>(v.content)) {
        dst.value()->value = std::move(v.get<Cell::boxed_type>());
    }
}

auto execute(STRUCT_AT const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_proxy(stack, op.instruction.out, ip);
    auto src = get_value(stack, op.instruction.lhs, ip);
    auto key = get_value(stack, op.instruction.rhs, ip);

    if (src.is_void()) {
        throw abort_execution{ip, "cannot struct_at out of void"};
    }
    if (key.is_void()) {
        throw abort_execution{ip, "cannot struct_at with a void key"};
    }

    auto& str = src.boxed_of<viua::vm::types::Struct>().value().get();
    auto k    = key.boxed_of<viua::vm::types::Atom>().value().get().content;
    auto& v   = str.at(k);

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    if (std::holds_alternative<int64_t>(v.content)) {
        v.content = std::make_unique<Signed_integer>(v.get<int64_t>());
    } else if (std::holds_alternative<uint64_t>(v.content)) {
        v.content = std::make_unique<Unsigned_integer>(v.get<uint64_t>());
    } else if (std::holds_alternative<float>(v.content)) {
        v.content = std::make_unique<Float_single>(v.get<float>());
    } else if (std::holds_alternative<double>(v.content)) {
        v.content = std::make_unique<Float_double>(v.get<double>());
    }

    using viua::vm::types::Cell;
    dst = v.get<Cell::boxed_type>()->reference_to();
}
auto execute(STRUCT_INSERT const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_proxy(stack, op.instruction.out, ip);
    auto key = get_value(stack, op.instruction.lhs, ip);
    auto src = get_proxy(stack, op.instruction.rhs, ip);

    if (key.is_void()) {
        throw abort_execution{ip, "cannot struct_insert with a void key"};
    }
    if (src.view().is_void()) {
        throw abort_execution{ip, "cannot struct_insert with a void value"};
    }

    if (not dst.view().holds<viua::vm::types::Struct>()) {
        throw abort_execution{ip,
                              "invalid destination operand for struct_insert"};
    }

    auto k    = key.boxed_of<viua::vm::types::Atom>().value().get().content;
    auto& str = dst.boxed_of<viua::vm::types::Struct>().value().get();
    str.insert(k, std::move(src.overwrite().value_cell()));
}
auto execute(STRUCT_REMOVE const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_value(stack, op.instruction.lhs, ip);
    auto key = get_proxy(stack, op.instruction.rhs, ip);

    if (key.view().is_void()) {
        throw abort_execution{ip, "cannot struct_remove with a void key"};
    }
    if (src.is_void()) {
        throw abort_execution{ip, "cannot struct_remove with a void value"};
    }

    auto k    = key.boxed_of<viua::vm::types::Atom>().value().get().content;
    auto& str = src.boxed_of<viua::vm::types::Struct>().value().get();

    auto v = str.remove(k);
    if (dst) {
        dst.value()->value = std::move(v);
    }
}

auto execute(REF const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_slot(op.instruction.in, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{ip, "cannot take pointer to void"};
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
}

auto execute(IF const op, Stack& stack, ip_type const ip) -> ip_type
{
    auto const condition = get_value(stack, op.instruction.out, ip);
    auto tt              = get_proxy(stack, op.instruction.in, ip);

    auto take_branch =
        (condition.holds<void>() or cast_to<uint64_t>(condition));
    auto const target =
        take_branch
            ? (stack.back().entry_address + cast_to<uint64_t>(tt.view()))
            : (ip + 1);

    tt.overwrite().make_void();
    return target;
}

auto execute(IO_SUBMIT const op, Stack& stack, ip_type const ip) -> void
{
    auto dst [[maybe_unused]] = get_proxy(stack, op.instruction.out, ip);
    auto port                 = get_value(stack, op.instruction.lhs, ip);
    auto req                  = get_value(stack, op.instruction.rhs, ip)
                   .boxed_of<viua::vm::types::Struct>();

    if (not port.holds<int64_t>()) {
        throw abort_execution{ip, "invalid I/O port"};
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
        auto const rd = stack.proc.core->io.schedule(
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
        auto const rd = stack.proc.core->io.schedule(
            port.get<int64_t>(), IORING_OP_WRITE, std::move(buf));
        dst = rd;
        break;
    }
    }
}
auto execute(IO_WAIT const op, Stack& stack, ip_type const ip) -> void
{
    auto dst = get_proxy(stack, op.instruction.out, ip);
    auto req = get_value(stack, op.instruction.lhs, ip);

    auto const want_id = req.get<uint64_t>();
    if (not stack.proc.core->io.requests.contains(want_id)) {
        io_uring_cqe* cqe{};
        do {
            io_uring_wait_cqe(&stack.proc.core->io.ring, &cqe);

            if (cqe->res == -1) {
                stack.proc.core->io.requests[cqe->user_data]->status =
                    IO_request::Status::Error;
            } else {
                auto& rd  = *stack.proc.core->io.requests[cqe->user_data];
                rd.status = IO_request::Status::Success;

                if (rd.opcode == IORING_OP_READ) {
                    rd.buffer.resize(cqe->res);
                } else if (rd.opcode == IORING_OP_WRITE) {
                    rd.buffer = rd.buffer.substr(cqe->res);
                }
            }

            io_uring_cqe_seen(&stack.proc.core->io.ring, cqe);
        } while (cqe->user_data != want_id);
    }

    dst = std::make_unique<types::String>(
        std::move(stack.proc.core->io.requests[want_id]->buffer));
    stack.proc.core->io.requests.erase(want_id);
}
auto execute(IO_SHUTDOWN const, Stack&, ip_type const) -> void
{}
auto execute(IO_CTL const, Stack&, ip_type const) -> void
{}
auto execute(IO_PEEK const, Stack&, ip_type const) -> void
{}

auto execute(ACTOR const op, Stack& stack, ip_type const ip) -> void
{
    auto fn_name = std::string{};
    auto fn_addr = size_t{};
    {
        auto const fn_index   = op.instruction.in.index;
        auto const& fn_offset = stack.frames.back().registers.at(fn_index);
        if (fn_offset.is_void()) {
            throw abort_execution{ip, "fn offset cannot be void"};
        }
        if (fn_offset.is_boxed()) {
            // FIXME only unboxed integers allowed for now
            throw abort_execution{ip, "fn offset cannot be boxed"};
        }

        std::tie(fn_name, fn_addr) =
            stack.proc.module.function_at(fn_offset.value.get<uint64_t>());

        get_proxy(stack, op.instruction.in, ip).overwrite().make_void();
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{ip, "invalid IP after call"};
    }

    auto const fr_entry = (fn_addr / sizeof(viua::arch::instruction_type));

    auto const pid     = stack.proc.core->spawn("", fr_entry);
    auto dst           = get_slot(op.instruction.out, stack, ip);
    dst.value()->value = std::make_unique<viua::vm::types::PID>(pid);
}
auto execute(SELF const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::PID>(stack.proc.pid);
    target.value = std::move(s);
}
}  // namespace viua::vm::ins

namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;
namespace {
auto dump_registers(std::vector<Value> const& registers,
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

        using viua::vm::types::Signed_integer;
        using viua::vm::types::Unsigned_integer;
        if (auto const& is = each.boxed_of<Signed_integer>(); is) {
            auto const x = is.value().get().value;
            TRACE_STREAM << "is " << std::hex << std::setw(16)
                         << std::setfill('0') << x << " " << std::dec << x
                         << '\n';
        } else if (auto const& is = each.boxed_of<Unsigned_integer>(); is) {
            auto const x = is.value().get().value;
            TRACE_STREAM << "iu " << std::hex << std::setw(16)
                         << std::setfill('0') << x << " " << std::dec << x
                         << '\n';
        } else if (each.is_boxed()) {
            auto const& value = each.boxed_value();
            TRACE_STREAM << value.type_name();
            value.as_trait<viua::vm::types::traits::To_string>(
                [](viua::vm::types::traits::To_string const& val) -> void {
                    TRACE_STREAM << " = " << val.to_string();
                });
            TRACE_STREAM << '\n';
            continue;
        }

        if (each.is_void()) {
            /* do nothing */
        } else if (each.holds<int64_t>()) {
            TRACE_STREAM << "is " << std::hex << std::setw(16)
                         << std::setfill('0') << each.value.get<int64_t>()
                         << " " << std::dec << each.value.get<int64_t>()
                         << '\n';
        } else if (each.holds<uint64_t>()) {
            TRACE_STREAM << "iu " << std::hex << std::setw(16)
                         << std::setfill('0') << each.value.get<uint64_t>()
                         << " " << std::dec << each.value.get<uint64_t>()
                         << '\n';
        } else if (each.holds<float>()) {
            auto const precision = std::cerr.precision();
            TRACE_STREAM
                << "fl " << std::hexfloat << each.value.get<float>() << " "
                << std::defaultfloat
                << std::setprecision(std::numeric_limits<float>::digits10 + 1)
                << each.value.get<float>() << '\n';
            TRACE_STREAM << std::setprecision(precision);
        } else if (each.holds<double>()) {
            auto const precision = std::cerr.precision();
            TRACE_STREAM
                << "db " << std::hexfloat << each.value.get<double>() << " "
                << std::defaultfloat
                << std::setprecision(std::numeric_limits<double>::digits10 + 1)
                << each.value.get<double>() << '\n';
            TRACE_STREAM << std::setprecision(precision);
        }
    }
}
}  // namespace
auto execute(EBREAK const, Stack& stack, ip_type const) -> void
{
    viua::TRACE_STREAM << "begin ebreak in process "
                       << stack.proc.pid.to_string() << viua::TRACE_STREAM.endl;

    viua::TRACE_STREAM << "  backtrace:" << viua::TRACE_STREAM.endl;
    for (auto i = size_t{0}; i < stack.frames.size(); ++i) {
        auto const& each = stack.frames.at(i);

        viua::TRACE_STREAM << "    #" << i << "  "
                           << stack.proc.module.elf_path.native() << "[.text+0x"
                           << std::hex << std::setw(8) << std::setfill('0')
                           << ((each.entry_address - stack.proc.module.ip_base)
                               * sizeof(viua::arch::instruction_type))
                           << std::dec << ']';
        viua::TRACE_STREAM << " return to ";
        if (each.return_address) {
            viua::TRACE_STREAM
                << stack.proc.module.elf_path.native() << "[.text+0x"
                << std::hex << std::setw(8) << std::setfill('0')
                << ((each.return_address - stack.proc.module.ip_base)
                    * sizeof(viua::arch::instruction_type))
                << std::dec << ']';
        } else {
            viua::TRACE_STREAM << "null";
        }
        viua::TRACE_STREAM << viua::TRACE_STREAM.endl;
    }

    viua::TRACE_STREAM << "  register contents:" << viua::TRACE_STREAM.endl;
    for (auto i = size_t{0}; i < stack.frames.size(); ++i) {
        auto const& each = stack.frames.at(i);

        viua::TRACE_STREAM << "    of #" << i << viua::TRACE_STREAM.endl;

        dump_registers(each.parameters, "p");
        dump_registers(each.registers, "l");
    }
    dump_registers(stack.args, "a");

    viua::TRACE_STREAM << "end ebreak in process " << stack.proc.pid.to_string()
                       << viua::TRACE_STREAM.endl;
}
}  // namespace viua::vm::ins
