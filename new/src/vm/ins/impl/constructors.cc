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
}  // namespace viua::vm::ins
