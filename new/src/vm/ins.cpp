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

template<typename Op, typename Trait>
auto execute_arithmetic_op(Op const op, Stack& stack, ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";

    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(std::get<int64_t>(lhs.value),
                                          rhs.template cast_to<int64_t>());
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(std::get<uint64_t>(lhs.value),
                                          rhs.template cast_to<uint64_t>());
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(std::get<float>(lhs.value),
                                          rhs.template cast_to<float>());
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(std::get<double>(lhs.value),
                                          rhs.template cast_to<double>());
    } else if (lhs.template has_trait<Trait>()) {
        out.value = lhs.boxed_value().template as_trait<Trait>(rhs.value);
    } else {
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";

    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(std::get<int64_t>(lhs.value),
                                          rhs.template cast_to<int64_t>());
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(std::get<uint64_t>(lhs.value),
                                          rhs.template cast_to<uint64_t>());
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(std::get<float>(lhs.value),
                                          rhs.template cast_to<float>());
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(std::get<double>(lhs.value),
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";

    if (lhs.is_boxed() or rhs.is_boxed()) {
        throw abort_execution{
            nullptr, "boxed values not supported for modulo operations"};
    }

    if (lhs.holds<int64_t>()) {
        out = std::get<int64_t>(lhs.value) % rhs.cast_to<int64_t>();
    } else if (lhs.holds<uint64_t>()) {
        out = std::get<uint64_t>(lhs.value) % rhs.cast_to<uint64_t>();
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

    out.value =
        (std::get<uint64_t>(lhs.value) << std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(BITSHR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value =
        (std::get<uint64_t>(lhs.value) >> std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(BITASHR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    auto const tmp = static_cast<int64_t>(std::get<uint64_t>(lhs.value));
    out.value = static_cast<uint64_t>(tmp >> std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
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

    out.value = (std::get<uint64_t>(lhs.value) & std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(BITOR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (std::get<uint64_t>(lhs.value) | std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(BITXOR const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    out.value = (std::get<uint64_t>(lhs.value) ^ std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(BITNOT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& in        = registers.at(op.instruction.in.index);

    out.value = ~std::get<uint64_t>(in.value);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.in.index))
               + "\n";
}

template<typename Op, typename Trait>
auto execute_cmp_op(Op const op, Stack& stack, ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& lhs       = registers.at(op.instruction.lhs.index);
    auto& rhs       = registers.at(op.instruction.rhs.index);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";

    if (lhs.template holds<int64_t>()) {
        out = typename Op::functor_type{}(std::get<int64_t>(lhs.value),
                                          std::get<int64_t>(rhs.value));
    } else if (lhs.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(std::get<uint64_t>(lhs.value),
                                          std::get<uint64_t>(rhs.value));
    } else if (lhs.template holds<float>()) {
        out = typename Op::functor_type{}(std::get<float>(lhs.value),
                                          std::get<float>(rhs.value));
    } else if (lhs.template holds<double>()) {
        out = typename Op::functor_type{}(std::get<double>(lhs.value),
                                          std::get<double>(rhs.value));
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
        auto const lv = std::get<uint64_t>(lhs.value);
        auto const rv = std::get<uint64_t>(rhs.value);
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
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
        auto const use_lhs = (std::get<uint64_t>(lhs.value) == 0);
        out                = use_lhs ? std::move(lhs) : std::move(rhs);
    }

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
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
        auto const use_lhs = (std::get<uint64_t>(lhs.value) != 0);
        out                = use_lhs ? std::move(lhs) : std::move(rhs);
    }

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
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
        out = static_cast<bool>(std::get<uint64_t>(in.value));
    }

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.in.index))
               + "\n";
}

namespace {
auto type_name(viua::vm::types::Register_cell const& c) -> std::string
{
    if (std::holds_alternative<std::monostate>(c)) {
        return "void";
    } else if (std::holds_alternative<int64_t>(c)) {
        return "int";
    } else if (std::holds_alternative<uint64_t>(c)) {
        return "uint";
    } else if (std::holds_alternative<float>(c)) {
        return "float";
    } else if (std::holds_alternative<double>(c)) {
        return "double";
    } else {
        return std::get<std::unique_ptr<viua::vm::types::Value>>(c)
            ->type_name();
    }
}
auto to_string(viua::arch::Register_access const ra) -> std::string
{
    auto out = std::ostringstream{};
    out << (ra.direct ? '$' : '*') << static_cast<int>(ra.index);
    switch (ra.set) {
        using enum viua::arch::RS;
    case VOID:
        return "void";
    case LOCAL:
        out << ".l";
        break;
    case ARGUMENT:
        out << ".a";
        break;
    case PARAMETER:
        out << ".p";
        break;
    }
    return out.str();
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
    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.in.index))
               + "\n";

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
        out = std::get<int64_t>(in.value);
    } else if (in.holds<uint64_t>()) {
        out = std::get<uint64_t>(in.value);
    } else if (in.holds<float>()) {
        out = std::get<float>(in.value);
    } else if (in.holds<double>()) {
        out = std::get<double>(in.value);
    } else {
        out.value = in.boxed_value().as_trait<Copy>().copy();
    }
}
auto execute(MOVE const op, Stack& stack, ip_type const ip) -> void
{
    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " " + to_string(op.instruction.out) + ", "
                     + to_string(op.instruction.in) + "\n";

    auto in  = get_slot(op.instruction.in, stack, ip);
    auto out = get_slot(op.instruction.out, stack, ip);

    if (not in.has_value()) {
        throw abort_execution{ip, "cannot move out of void"};
    }
    if (out.has_value()) {
        **out = std::move(**in);
    }
    (*in)->value = std::monostate{};
}
auto execute(SWAP const op, Stack& stack, ip_type const) -> void
{
    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.in.index))
               + "\n";

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
    auto const data_offset = std::get<uint64_t>(target.value);
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}
auto execute(STRING const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto const& env        = stack.environment;
    auto const data_offset = std::get<uint64_t>(target.value);
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}

auto execute(FRAME const op, Stack& stack, ip_type const ip) -> void
{
    auto const index = op.instruction.out.index;
    auto const rs    = op.instruction.out.set;

    auto capacity = viua::arch::register_index_type{};
    if (rs == viua::arch::RS::LOCAL) {
        capacity = static_cast<viua::arch::register_index_type>(
            std::get<uint64_t>(stack.back().registers.at(index).value));
    } else if (rs == viua::arch::RS::ARGUMENT) {
        capacity = index;
    } else {
        throw abort_execution{
            ip, "args count must come from local or argument register set"};
    }

    stack.args = std::vector<Value>(capacity);

    std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
                     + " $" + std::to_string(static_cast<int>(index)) + '.'
                     + ((rs == viua::arch::RS::LOCAL)      ? 'l'
                        : (rs == viua::arch::RS::ARGUMENT) ? 'a'
                                                           : 'p')
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
            stack.environment.function_at(std::get<uint64_t>(fn_offset.value));
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", " + std::to_string(op.instruction.immediate) + "\n";
}
auto execute(LUIU const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& value     = registers.at(op.instruction.out.index);
    value.value     = (op.instruction.immediate << 28);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", " + std::to_string(op.instruction.immediate) + "\n";
}

auto execute(FLOAT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;

    auto& target = registers.at(op.instruction.out.index);

    auto const data_offset = std::get<uint64_t>(target.value);
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}
auto execute(DOUBLE const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;

    auto& target = registers.at(op.instruction.out.index);

    auto const data_offset = std::get<uint64_t>(target.value);
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

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}
auto execute(STRUCT const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::Struct>();
    target.value = std::move(s);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}
auto execute(BUFFER const op, Stack& stack, ip_type const) -> void
{
    auto& registers = stack.back().registers;
    auto& target    = registers.at(op.instruction.out.index);

    auto s       = std::make_unique<viua::vm::types::Buffer>();
    target.value = std::move(s);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}

template<typename Op>
auto execute_arithmetic_immediate_op(Op const op,
                                     Stack& stack,
                                     ip_type const ip) -> void
{
    auto& registers = stack.frames.back().registers;
    auto& out       = registers.at(op.instruction.out.index);
    auto& in        = registers.at(op.instruction.in.index);

    constexpr auto const signed_immediate =
        std::is_signed_v<typename Op::value_type>;
    using immediate_type =
        std::conditional<signed_immediate, int64_t, uint64_t>::type;
    auto const immediate =
        (signed_immediate
             ? static_cast<immediate_type>(
                 static_cast<int32_t>(op.instruction.immediate << 8) >> 8)
             : static_cast<immediate_type>(op.instruction.immediate));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", "
               + (in.template holds<void>()
                      ? "void"
                      : ('$'
                         + std::to_string(
                             static_cast<int>(op.instruction.in.index))))
               + ", " + std::to_string(op.instruction.immediate)
               + (signed_immediate ? "" : "u") + "\n";

    if (in.template holds<void>()) {
        out = typename Op::functor_type{}(0, immediate);
    } else if (in.template holds<uint64_t>()) {
        out = typename Op::functor_type{}(std::get<uint64_t>(in.value),
                                          immediate);
    } else if (in.template holds<int64_t>()) {
        out =
            typename Op::functor_type{}(std::get<int64_t>(in.value), immediate);
    } else if (in.template holds<float>()) {
        out = typename Op::functor_type{}(std::get<float>(in.value), immediate);
    } else if (in.template holds<double>()) {
        out =
            typename Op::functor_type{}(std::get<double>(in.value), immediate);
    } else {
        throw abort_execution{
            ip,
            "unsupported operand type for immediate arithmetic operation: "
                + type_name(in.value)};
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
                    << std::get<uint64_t>(each.value) << " " << std::dec
                    << static_cast<int64_t>(std::get<uint64_t>(each.value))
                    << "\n";
            } else if (each.holds<uint64_t>()) {
                std::cerr << "iu " << std::hex << std::setw(16)
                          << std::setfill('0') << std::get<uint64_t>(each.value)
                          << " " << std::dec << std::get<uint64_t>(each.value)
                          << "\n";
            } else if (each.holds<float>()) {
                std::cerr << "fl " << std::hex << std::setw(8)
                          << std::setfill('0')
                          << static_cast<float>(std::get<uint64_t>(each.value))
                          << " " << std::dec
                          << static_cast<float>(std::get<uint64_t>(each.value))
                          << "\n";
            } else if (each.holds<double>()) {
                std::cerr << "db " << std::hex << std::setw(16)
                          << std::setfill('0')
                          << static_cast<double>(std::get<uint64_t>(each.value))
                          << " " << std::dec
                          << static_cast<double>(std::get<uint64_t>(each.value))
                          << "\n";
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
                    << std::get<uint64_t>(each.value) << " " << std::dec
                    << static_cast<int64_t>(std::get<uint64_t>(each.value))
                    << "\n";
            } else if (each.holds<uint64_t>()) {
                std::cerr << "iu " << std::hex << std::setw(16)
                          << std::setfill('0') << std::get<uint64_t>(each.value)
                          << " " << std::dec << std::get<uint64_t>(each.value)
                          << "\n";
            } else if (each.holds<float>()) {
                std::cerr << "fl " << std::hex << std::setw(8)
                          << std::setfill('0')
                          << static_cast<float>(std::get<uint64_t>(each.value))
                          << " " << std::dec
                          << static_cast<float>(std::get<uint64_t>(each.value))
                          << "\n";
            } else if (each.holds<double>()) {
                std::cerr << "db " << std::hex << std::setw(16)
                          << std::setfill('0')
                          << static_cast<double>(std::get<uint64_t>(each.value))
                          << " " << std::dec
                          << static_cast<double>(std::get<uint64_t>(each.value))
                          << "\n";
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
                          << std::setfill('0') << std::get<int64_t>(each.value)
                          << " " << std::dec << std::get<int64_t>(each.value)
                          << "\n";
            } else if (each.holds<uint64_t>()) {
                std::cerr << "iu " << std::hex << std::setw(16)
                          << std::setfill('0') << std::get<uint64_t>(each.value)
                          << " " << std::dec << std::get<uint64_t>(each.value)
                          << "\n";
            } else if (each.holds<float>()) {
                auto const precision = std::cerr.precision();
                std::cerr << "fl " << std::hexfloat
                          << std::get<float>(each.value) << " "
                          << std::defaultfloat
                          << std::setprecision(
                                 std::numeric_limits<float>::digits10 + 1)
                          << std::get<float>(each.value) << "\n";
                std::cerr << std::setprecision(precision);
            } else if (each.holds<double>()) {
                auto const precision = std::cerr.precision();
                std::cerr << "db " << std::hexfloat
                          << std::get<double>(each.value) << " "
                          << std::defaultfloat
                          << std::setprecision(
                                 std::numeric_limits<double>::digits10 + 1)
                          << std::get<double>(each.value) << "\n";
                std::cerr << std::setprecision(precision);
            }
        }
    }
}

}  // namespace viua::vm::ins
