/*
 *  Copyright (C) 2021 Marek Marecki
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
    if (lhs.template holds<int64_t>()) {
        auto r = typename Op::functor_type{}(lhs.template get<int64_t>(),
                                             cast_to<int64_t>(rhs));
        if (out.hard()) {
            out = std::move(r);
        } else {
            throw abort_execution{
                ip,
                ("cannot store i64 through a reference to " + type_name(out))};
        }
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(lhs.template get<uint64_t>(),
                                          cast_to<uint64_t>(rhs));
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(lhs.template get<float>(),
                                          cast_to<float>(rhs));
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(lhs.template get<double>(),
                                          cast_to<double>(rhs));
    } else if (lhs.template holds<Signed_integer>()) {
        auto r = typename Op::functor_type{}(cast_to<int64_t>(lhs),
                                             cast_to<int64_t>(rhs));

        if (out.hard()) {
            out = std::move(r);
        } else if (not out.view().template holds<Signed_integer>()) {
            throw abort_execution{
                ip,
                ("cannot store i64 through a reference to " + type_name(out))};
        } else {
            auto b = out.view().template boxed_of<Signed_integer>();
            b.value().get().value = std::move(r);
        }
    } else if (lhs.template holds<Unsigned_integer>()) {
        out = typename Op::functor_type{}(cast_to<uint64_t>(lhs),
                                          cast_to<uint64_t>(rhs));
    } else if (lhs.template holds<Float_single>()) {
        out = typename Op::functor_type{}(cast_to<float>(lhs),
                                          cast_to<float>(rhs));
    } else if (lhs.template holds<Float_double>()) {
        out = typename Op::functor_type{}(cast_to<double>(lhs),
                                          cast_to<double>(rhs));
    } /* else if (lhs.template has_trait<Trait>()) {
        out.value = lhs.boxed_value().template as_trait<Trait>(rhs.content);
    } */
    else {
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

    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(lhs.template get<int64_t>(),
                                          cast_to<int64_t>(rhs));
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(lhs.template get<uint64_t>(),
                                          cast_to<uint64_t>(rhs));
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(lhs.template get<float>(),
                                          cast_to<float>(rhs));
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(lhs.template get<double>(),
                                          cast_to<double>(rhs));
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
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    if (lhs.is_boxed() or rhs.is_boxed()) {
        throw abort_execution{
            nullptr, "boxed values not supported for modulo operations"};
    }

    if (lhs.holds<int64_t>()) {
        out = lhs.value.get<int64_t>() % rhs.cast_to<int64_t>();
    } else if (lhs.holds<uint64_t>()) {
        out = lhs.value.get<uint64_t>() % rhs.cast_to<uint64_t>();
    } else {
        throw abort_execution{ip,
                              "unsupported operand types for modulo operation"};
    }
}

auto execute(BITSHL const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_slot(op.instruction.out, stack, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    *out.value() = (lhs.get<uint64_t>() << rhs.get<uint64_t>());
}
auto execute(BITSHR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_slot(op.instruction.out, stack, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    *out.value() = (lhs.get<uint64_t>() >> rhs.get<uint64_t>());
}
auto execute(BITASHR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_slot(op.instruction.out, stack, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    auto const tmp = static_cast<int64_t>(lhs.get<uint64_t>());
    *out.value()   = static_cast<uint64_t>(tmp >> rhs.get<uint64_t>());
}
auto execute(BITROL const, Stack&, ip_type const) -> void
{}
auto execute(BITROR const, Stack&, ip_type const) -> void
{}
auto execute(BITAND const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_slot(op.instruction.out, stack, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    *out.value() = (lhs.get<uint64_t>() & rhs.get<uint64_t>());
}
auto execute(BITOR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_slot(op.instruction.out, stack, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    *out.value() = (lhs.get<uint64_t>() | rhs.get<uint64_t>());
}
auto execute(BITXOR const op, Stack& stack, ip_type const ip) -> void
{
    auto out = get_slot(op.instruction.out, stack, ip);
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    *out.value() = (lhs.get<uint64_t>() ^ rhs.get<uint64_t>());
}
auto execute(BITNOT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& in        = registers.at(op.instruction.in.index);

    out.value = ~in.value.get<uint64_t>();
}

auto execute(EQ const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Eq;
    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    using viua::vm::types::String;
    using viua::vm::types::Atom;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Eq>()) {
        auto const& cmp = lhs.boxed_of<Eq>().value().get();
        cmp_result = cmp(cmp, rhs);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for eq"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot eq unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out = (cmp_result == 0);
}
auto execute(LT const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    using viua::vm::types::String;
    using viua::vm::types::Atom;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for lt"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot lt unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out = (cmp_result < 0);
}
auto execute(GT const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    using viua::vm::types::String;
    using viua::vm::types::Atom;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for gt"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot gt unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out = (cmp_result > 0);
}
auto execute(CMP const op, Stack& stack, ip_type const ip) -> void
{
    auto lhs = get_value(stack, op.instruction.lhs, ip);
    auto rhs = get_value(stack, op.instruction.rhs, ip);

    using viua::vm::types::traits::Cmp;
    auto cmp_result = std::partial_ordering::unordered;

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    using viua::vm::types::String;
    using viua::vm::types::Atom;
    if (lhs.holds<int64_t>()) {
        auto const l = lhs.get<int64_t>();
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<uint64_t>()) {
        auto const l = lhs.get<uint64_t>();
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<float>()) {
        auto const l = lhs.get<float>();
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<double>()) {
        auto const l = lhs.get<double>();
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Signed_integer>()) {
        auto const l = cast_to<int64_t>(lhs);
        auto const r = cast_to<int64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Unsigned_integer>()) {
        auto const l = cast_to<uint64_t>(lhs);
        auto const r = cast_to<uint64_t>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_single>()) {
        auto const l = cast_to<float>(lhs);
        auto const r = cast_to<float>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Float_double>()) {
        auto const l = cast_to<double>(lhs);
        auto const r = cast_to<double>(rhs);
        cmp_result = (l <=> r);
    } else if (lhs.holds<Cmp>()) {
        auto const& cmp = lhs.boxed_of<Cmp>().value().get();
        cmp_result = cmp(cmp, rhs);
    } else {
        throw abort_execution{ip, "invalid operands for cmp"};
    }

    if (cmp_result == std::partial_ordering::unordered) {
        throw abort_execution{ip, "cannot cmp unordered values"};
    }

    auto out = get_proxy(stack, op.instruction.out, ip);
    out = (cmp_result < 0) ? -1 : (0 < cmp_result) ? 1 : 0;
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

    auto const& env        = stack.environment;
    auto const data_offset = target.value.get<uint64_t>();
    auto const data_size   = [env, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &env.strings_table[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto s     = std::make_unique<viua::vm::types::Atom>();
    s->content = std::string{
        reinterpret_cast<char const*>(&env.strings_table[0] + data_offset),
        data_size};

    target.value = std::move(s);
}
auto execute(STRING const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto const& env        = stack.environment;
    auto const data_offset = target.value.get<uint64_t>();
    auto const data_size   = [env, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &env.strings_table[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto s     = std::make_unique<viua::vm::types::String>();
    s->content = std::string{
        reinterpret_cast<char const*>(&env.strings_table[0] + data_offset),
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
            stack.environment.function_at(fn_offset.value.get<uint64_t>());
    }

    if (fn_addr % sizeof(viua::arch::instruction_type)) {
        throw abort_execution{ip, "invalid IP after call"};
    }

    auto const fr_return = (ip + 1);
    auto const fr_entry  = (stack.environment.ip_base
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
               &stack.environment.strings_table[size_offset],
               sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint32_t{};
    memcpy(
        &tmp, (&stack.environment.strings_table[0] + data_offset), data_size);
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
               &stack.environment.strings_table[size_offset],
               sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto tmp = uint64_t{};
    memcpy(
        &tmp, (&stack.environment.strings_table[0] + data_offset), data_size);
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
    auto k    = key.boxed_of<viua::vm::types::Atom>().value().get().to_string();
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

    auto k    = key.boxed_of<viua::vm::types::Atom>().value().get().to_string();
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

    auto k    = key.boxed_of<viua::vm::types::Atom>().value().get().to_string();
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
}  // namespace viua::vm::ins

namespace viua {
extern viua::support::fdstream TRACE_STREAM;
}

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

        TRACE_STREAM << "  " << std::setw(7) << std::setfill(' ')
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
    dump_registers(stack.args, "a");
    dump_registers(stack.frames.back().parameters, "p");
    dump_registers(stack.frames.back().registers, "l");
}
}  // namespace viua::vm::ins
