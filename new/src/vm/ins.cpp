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
#include <viua/vm/ins.h>


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;
using ip_type = viua::arch::instruction_type const*;

auto get_value(std::vector<viua::vm::Value>& registers, size_t const i)
    -> viua::vm::types::Cell_view
{
    using viua::vm::types::Cell;
    using viua::vm::types::Cell_view;

    auto& c = registers.at(i);
    if (std::holds_alternative<Cell::boxed_type>(c.value.content)) {
        auto& b = std::get<Cell::boxed_type>(c.value.content);
        if (auto p = dynamic_cast<types::Pointer*>(b.get()); p) {
            return Cell_view{*p->value};
        }
    }

    return c.value.view();
}
auto get_value(std::vector<viua::vm::Value>& registers,
               viua::arch::Register_access const a)
    -> viua::vm::types::Cell_view
{
    using viua::vm::types::Cell;
    using viua::vm::types::Cell_view;

    auto& c = registers.at(a.index);
    if (std::holds_alternative<Cell::boxed_type>(c.value.content)) {
        auto& b = std::get<Cell::boxed_type>(c.value.content);
        if (auto p = dynamic_cast<types::Pointer*>(b.get());
            p and not a.direct) {
            return Cell_view{*p->value};
        }
    }

    return c.value.view();
}

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

    /*
    if (holds<Signed_integer>()) {
        return static_cast<T>(
            static_cast<Signed_integer const&>(boxed_value()).value);
    }
    if (holds<Unsigned_integer>()) {
        return static_cast<T>(
            static_cast<Unsigned_integer const&>(boxed_value()).value);
    }
    if (holds<Float_single>()) {
        return static_cast<T>(
            static_cast<Float_single const&>(boxed_value()).value);
    }
    if (holds<Float_double>()) {
        return static_cast<T>(
            static_cast<Float_double const&>(boxed_value()).value);
    }
    */

    throw std::bad_cast{};
}

template<typename Op, typename Trait>
auto execute_arithmetic_op(Op const op, Stack& stack, ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto rhs        = get_value(registers, op.instruction.rhs.index);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    using viua::vm::types::Float_double;
    using viua::vm::types::Float_single;
    using viua::vm::types::Signed_integer;
    using viua::vm::types::Unsigned_integer;
    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(lhs.value.template get<int64_t>(),
                                          cast_to<int64_t>(rhs));
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(lhs.value.template get<uint64_t>(),
                                          cast_to<uint64_t>(rhs));
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(lhs.value.template get<float>(),
                                          cast_to<float>(rhs));
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(lhs.value.template get<double>(),
                                          cast_to<double>(rhs));
    } else if (lhs.template holds<Signed_integer>()) {
        out = typename Op::functor_type{}(lhs.template cast_to<int64_t>(),
                                          cast_to<int64_t>(rhs));
    } else if (lhs.template holds<Unsigned_integer>()) {
        out = typename Op::functor_type{}(lhs.template cast_to<uint64_t>(),
                                          cast_to<uint64_t>(rhs));
    } else if (lhs.template holds<Float_single>()) {
        out = typename Op::functor_type{}(lhs.template cast_to<float>(),
                                          cast_to<float>(rhs));
    } else if (lhs.template holds<Float_double>()) {
        out = typename Op::functor_type{}(lhs.template cast_to<double>(),
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
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(lhs.value.template get<int64_t>(),
                                          rhs.template cast_to<int64_t>());
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(lhs.value.template get<uint64_t>(),
                                          rhs.template cast_to<uint64_t>());
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(lhs.value.template get<float>(),
                                          rhs.template cast_to<float>());
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(lhs.value.template get<double>(),
                                          rhs.template cast_to<double>());
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

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

auto execute(BITSHL const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (lhs.value.get<uint64_t>() << rhs.value.get<uint64_t>());

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}
auto execute(BITSHR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (lhs.value.get<uint64_t>() >> rhs.value.get<uint64_t>());

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}
auto execute(BITASHR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    auto const tmp = static_cast<int64_t>(lhs.value.get<uint64_t>());
    out.value      = static_cast<uint64_t>(tmp >> rhs.value.get<uint64_t>());

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}
auto execute(BITROL const, Stack&, ip_type const) -> void
{}
auto execute(BITROR const, Stack&, ip_type const) -> void
{}
auto execute(BITAND const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (lhs.value.get<uint64_t>() & rhs.value.get<uint64_t>());

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}
auto execute(BITOR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (lhs.value.get<uint64_t>() | rhs.value.get<uint64_t>());

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}
auto execute(BITXOR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (lhs.value.get<uint64_t>() ^ rhs.value.get<uint64_t>());

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}
auto execute(BITNOT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& in        = registers.at(op.instruction.in.index);

    out.value = ~in.value.get<uint64_t>();

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";
}

template<typename Op, typename Trait>
auto execute_cmp_op(Op const op, Stack& stack, ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(lhs.value.template get<int64_t>(),
                                          rhs.value.template get<int64_t>());
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(lhs.value.template get<uint64_t>(),
                                          rhs.value.template get<uint64_t>());
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(lhs.value.template get<float>(),
                                          rhs.value.template get<float>());
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(lhs.value.template get<double>(),
                                          rhs.value.template get<double>());
    } else if (lhs.template has_trait<Trait>()) {
        out.value = lhs.boxed_value().template as_trait<Trait>(rhs.value);
    } else {
        throw abort_execution{
            ip, "unsupported operand types for compare operation"};
    }
}
auto execute(EQ const op, Stack& stack, ip_type const ip) -> void
{
    execute_cmp_op<EQ, viua::vm::types::traits::Eq>(op, stack, ip);
}
auto execute(LT const op, Stack& stack, ip_type const ip) -> void
{
    execute_cmp_op<LT, viua::vm::types::traits::Lt>(op, stack, ip);
}
auto execute(GT const op, Stack& stack, ip_type const ip) -> void
{
    execute_cmp_op<GT, viua::vm::types::traits::Gt>(op, stack, ip);
}
auto execute(CMP const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    if ((not lhs.is_boxed()) and rhs.is_boxed()) {
        throw abort_execution{nullptr,
                              "cmp: unboxed lhs cannot be used with boxed rhs"};
    }

    using viua::vm::types::traits::Cmp;
    if (lhs.is_boxed()) {
        auto const& lhs_value = lhs.boxed_value();

        if (lhs_value.has_trait<Cmp>()) {
            out = lhs_value.as_trait<Cmp, int64_t>(
                [&rhs](Cmp const& val) -> int64_t {
                    auto const& rv = rhs.boxed_value();
                    return val.cmp(rv);
                },
                Cmp::CMP_LT);
        } else {
            out = Cmp::CMP_LT;
        }
    } else {
        auto const lv = lhs.value.get<uint64_t>();
        auto const rv = rhs.value.get<uint64_t>();
        if (lv == rv) {
            out = Cmp::CMP_EQ;
        } else if (lv > rv) {
            out = Cmp::CMP_GT;
        } else if (lv < rv) {
            out = Cmp::CMP_LT;
        } else {
            throw abort_execution{nullptr, "cmp: incomparable unboxed values"};
        }
    }

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";
}

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
}  // namespace
auto execute(COPY const op, Stack& stack, ip_type const) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";

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
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";

    auto in  = get_slot(op.instruction.in, stack, ip);
    auto out = get_slot(op.instruction.out, stack, ip);

    if (not in.has_value()) {
        throw abort_execution{ip, "cannot move out of void"};
    }
    if (out.has_value()) {
        **out = std::move(**in);
    }
    in.value()->make_void();
}
auto execute(SWAP const op, Stack& stack, ip_type const) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";

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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + "\n";
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + "\n";
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string()
                     + " (frame with args capacity " + std::to_string(capacity)
                     + ")" + "\n";
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

    std::cerr << "    " << viua::arch::ops::to_string(op.instruction.opcode)
              << ", " << fn_name << " (at +0x" << std::hex << std::setw(8)
              << std::setfill('0') << fn_addr << std::dec << ")"
              << "\n";

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

    std::cerr << "return to " << std::hex << std::setw(16) << std::setfill('0')
              << fr.return_address << std::dec << "\n";

    return fr.return_address;
}

auto execute(LUI const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& value     = registers.at(op.instruction.out.index);
    value.value     = static_cast<int64_t>(op.instruction.immediate << 28);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + std::to_string(op.instruction.immediate) + "\n";
}
auto execute(LUIU const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& value     = registers.at(op.instruction.out.index);
    value.value     = (op.instruction.immediate << 28);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + std::to_string(op.instruction.immediate) + "\n";
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

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + "\n";
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

    auto tmp = uint32_t{};
    memcpy(
        &tmp, (&stack.environment.strings_table[0] + data_offset), data_size);
    tmp = le32toh(tmp);

    auto v = double{};
    memcpy(&v, &tmp, data_size);

    target.value = v;

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + "\n";
}
auto execute(STRUCT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::Struct>();
    target.value = std::move(s);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + "\n";
}
auto execute(BUFFER const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::Buffer>();
    target.value = std::move(s);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + "\n";
}

template<typename Op>
auto execute_arithmetic_immediate_op(Op const op,
                                     Stack& stack,
                                     ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto in         = get_value(registers, op.instruction.in);

    constexpr auto const signed_immediate =
        std::is_signed_v<typename Op::value_type>;
    using immediate_type =
        std::conditional<signed_immediate, int64_t, uint64_t>::type;
    auto const immediate =
        (signed_immediate
             ? static_cast<immediate_type>(
                 static_cast<int32_t>(op.instruction.immediate << 8) >> 8)
             : static_cast<immediate_type>(op.instruction.immediate));

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + ", "
                     + std::to_string(op.instruction.immediate)
                     + (signed_immediate ? "" : "u") + "\n";

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
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";

    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_slot(op.instruction.in, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{ip, "cannot buffer_push out of void"};
    }

    if (not dst.value()->holds<viua::vm::types::Buffer>()) {
        throw abort_execution{ip,
                              "invalid destination operand for buffer_push"};
    }
    static_cast<viua::vm::types::Buffer&>(dst.value()->boxed_value())
        .push(std::move(src.value()->value_cell()));
}
auto execute(BUFFER_SIZE const op, Stack& stack, ip_type const ip) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";

    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_slot(op.instruction.in, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{ip, "cannot take buffer_size of void"};
    }
    *dst.value() =
        static_cast<viua::vm::types::Buffer&>(src.value()->boxed_value())
            .size();
}
auto execute(BUFFER_AT const op, Stack& stack, ip_type const ip) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_slot(op.instruction.lhs, stack, ip);
    auto idx = get_slot(op.instruction.rhs, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{ip, "cannot buffer_at out of void"};
    }

    auto& buf =
        static_cast<viua::vm::types::Buffer&>(src.value()->boxed_value());
    auto off = (buf.size() - 1);
    if (idx.has_value()) {
        off = idx.value()->value.get<uint64_t>();
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
    dst.value()->value = v.get<Cell::boxed_type>()->pointer_to();
}
auto execute(BUFFER_POP const op, Stack& stack, ip_type const ip) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    auto dst = get_slot(op.instruction.out, stack, ip);
    auto src = get_slot(op.instruction.lhs, stack, ip);
    auto idx = get_slot(op.instruction.rhs, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{ip, "cannot buffer_pop out of void"};
    }

    auto& buf =
        static_cast<viua::vm::types::Buffer&>(src.value()->boxed_value());
    auto off = (buf.size() - 1);
    if (idx.has_value()) {
        off = idx.value()->value.get<uint64_t>();
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
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    auto dst      = get_slot(op.instruction.out, stack, ip);
    auto src      = get_slot(op.instruction.lhs, stack, ip);
    auto key_slot = get_slot(op.instruction.rhs, stack, ip);

    if (not src.has_value()) {
        throw abort_execution{ip, "cannot struct_at out of void"};
    }
    if (not key_slot.has_value()) {
        throw abort_execution{ip, "cannot struct_at with a void key"};
    }

    auto& str =
        static_cast<viua::vm::types::Struct&>(src.value()->boxed_value());
    auto key =
        static_cast<viua::vm::types::Atom&>(key_slot.value()->boxed_value())
            .to_string();
    auto& v = str.at(key);

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
    dst.value()->value = v.get<Cell::boxed_type>()->pointer_to();
}
auto execute(STRUCT_INSERT const op, Stack& stack, ip_type const ip) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";

    auto dst      = get_slot(op.instruction.out, stack, ip);
    auto key_slot = get_slot(op.instruction.lhs, stack, ip);
    auto src      = get_slot(op.instruction.rhs, stack, ip);

    if (not key_slot.has_value()) {
        throw abort_execution{ip, "cannot struct_insert with a void key"};
    }
    if (not src.has_value()) {
        throw abort_execution{ip, "cannot struct_insert with a void value"};
    }

    if (not dst.value()->holds<viua::vm::types::Struct>()) {
        throw abort_execution{ip,
                              "invalid destination operand for struct_insert"};
    }

    auto key =
        static_cast<viua::vm::types::Atom&>(key_slot.value()->boxed_value())
            .to_string();
    static_cast<viua::vm::types::Struct&>(dst.value()->boxed_value())
        .insert(key, std::move(src.value()->value_cell()));
}
auto execute(STRUCT_REMOVE const op, Stack&, ip_type const) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.lhs.to_string() + ", "
                     + op.instruction.rhs.to_string() + "\n";
}

auto execute(PTR const op, Stack& stack, ip_type const ip) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + op.instruction.out.to_string() + ", "
                     + op.instruction.in.to_string() + "\n";

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

    dst.value()->value = src.value()->boxed_value().pointer_to();
}

auto execute(EBREAK const, Stack& stack, ip_type const) -> void
{
    std::cerr << "[ebreak.stack]\n";
    std::cerr << "  depth = " << stack.frames.size() << "\n";

    {
        std::cerr << "[ebreak.arguments]\n";
        auto const& registers = stack.args;
        for (auto i = size_t{0}; i < registers.size(); ++i) {
            auto const& each = registers.at(i);
            if (each.is_void()) {
                continue;
            }

            std::cerr << "  [" << std::setw(3) << i << "] ";

            if (each.is_boxed()) {
                auto const& value = each.boxed_value();
                std::cerr << "<boxed> " << value.type_name();
                value.as_trait<viua::vm::types::traits::To_string>(
                    [](viua::vm::types::traits::To_string const& val) -> void {
                        std::cerr << " = " << val.to_string();
                    });
                std::cerr << "\n";
                continue;
            }

            if (each.is_void()) {
                /* do nothing */
            } else if (each.holds<int64_t>()) {
                std::cerr
                    << "is " << std::hex << std::setw(16) << std::setfill('0')
                    << each.value.get<uint64_t>() << " " << std::dec
                    << static_cast<int64_t>(each.value.get<uint64_t>()) << "\n";
            } else if (each.holds<uint64_t>()) {
                std::cerr << "iu " << std::hex << std::setw(16)
                          << std::setfill('0') << each.value.get<uint64_t>()
                          << " " << std::dec << each.value.get<uint64_t>()
                          << "\n";
            } else if (each.holds<float>()) {
                std::cerr
                    << "fl " << std::hex << std::setw(8) << std::setfill('0')
                    << static_cast<float>(each.value.get<uint64_t>()) << " "
                    << std::dec
                    << static_cast<float>(each.value.get<uint64_t>()) << "\n";
            } else if (each.holds<double>()) {
                std::cerr
                    << "db " << std::hex << std::setw(16) << std::setfill('0')
                    << static_cast<double>(each.value.get<uint64_t>()) << " "
                    << std::dec
                    << static_cast<double>(each.value.get<uint64_t>()) << "\n";
            }
        }
    }
    {
        std::cerr << "[ebreak.parameters]\n";
        auto const& registers = stack.frames.back().parameters;
        for (auto i = size_t{0}; i < registers.size(); ++i) {
            auto const& each = registers.at(i);
            if (each.is_void()) {
                continue;
            }

            std::cerr << "  [" << std::setw(3) << i << "] ";

            if (each.is_boxed()) {
                auto const& value = each.boxed_value();
                std::cerr << "<boxed> " << value.type_name();
                value.as_trait<viua::vm::types::traits::To_string>(
                    [](viua::vm::types::traits::To_string const& val) -> void {
                        std::cerr << " = " << val.to_string();
                    });
                std::cerr << "\n";
                continue;
            }

            if (each.is_void()) {
                /* do nothing */
            } else if (each.holds<int64_t>()) {
                std::cerr
                    << "is " << std::hex << std::setw(16) << std::setfill('0')
                    << each.value.get<uint64_t>() << " " << std::dec
                    << static_cast<int64_t>(each.value.get<uint64_t>()) << "\n";
            } else if (each.holds<uint64_t>()) {
                std::cerr << "iu " << std::hex << std::setw(16)
                          << std::setfill('0') << each.value.get<uint64_t>()
                          << " " << std::dec << each.value.get<uint64_t>()
                          << "\n";
            } else if (each.holds<float>()) {
                std::cerr
                    << "fl " << std::hex << std::setw(8) << std::setfill('0')
                    << static_cast<float>(each.value.get<uint64_t>()) << " "
                    << std::dec
                    << static_cast<float>(each.value.get<uint64_t>()) << "\n";
            } else if (each.holds<double>()) {
                std::cerr
                    << "db " << std::hex << std::setw(16) << std::setfill('0')
                    << static_cast<double>(each.value.get<uint64_t>()) << " "
                    << std::dec
                    << static_cast<double>(each.value.get<uint64_t>()) << "\n";
            }
        }
    }
    {
        std::cerr << "[ebreak.registers]\n";
        auto const& registers = stack.frames.back().registers;
        for (auto i = size_t{0}; i < registers.size(); ++i) {
            auto const& each = registers.at(i);
            if (each.is_void()) {
                continue;
            }

            std::cerr << "  [" << std::setw(3) << i << "] ";

            if (each.is_boxed()) {
                auto const& value = each.boxed_value();
                std::cerr << "<boxed> " << value.type_name();
                value.as_trait<viua::vm::types::traits::To_string>(
                    [](viua::vm::types::traits::To_string const& val) -> void {
                        std::cerr << " = " << val.to_string();
                    });
                std::cerr << "\n";
                continue;
            }

            if (each.is_void()) {
                /* do nothing */
            } else if (each.holds<int64_t>()) {
                std::cerr << "is " << std::hex << std::setw(16)
                          << std::setfill('0') << each.value.get<int64_t>()
                          << " " << std::dec << each.value.get<int64_t>()
                          << "\n";
            } else if (each.holds<uint64_t>()) {
                std::cerr << "iu " << std::hex << std::setw(16)
                          << std::setfill('0') << each.value.get<uint64_t>()
                          << " " << std::dec << each.value.get<uint64_t>()
                          << "\n";
            } else if (each.holds<float>()) {
                auto const precision = std::cerr.precision();
                std::cerr << "fl " << std::hexfloat << each.value.get<float>()
                          << " " << std::defaultfloat
                          << std::setprecision(
                                 std::numeric_limits<float>::digits10 + 1)
                          << each.value.get<float>() << "\n";
                std::cerr << std::setprecision(precision);
            } else if (each.holds<double>()) {
                auto const precision = std::cerr.precision();
                std::cerr << "db " << std::hexfloat << each.value.get<double>()
                          << " " << std::defaultfloat
                          << std::setprecision(
                                 std::numeric_limits<double>::digits10 + 1)
                          << each.value.get<double>() << "\n";
                std::cerr << std::setprecision(precision);
            }
        }
    }
}

}  // namespace viua::vm::ins
