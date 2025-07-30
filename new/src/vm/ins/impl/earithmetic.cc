/*
 *  Copyright (C) 2025 Marek Marecki
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
    EARITHMETICSTYLE const op,
    Stack& stack,
    ip_type const) -> void
{
    auto const out = mutable_proxy(stack, op.instruction.out);
    auto const in  = immutable_proxy(stack, op.instruction.in);

    auto const current_style = stack.proc->arithmetic_style;
    out                      = static_cast<uint64_t>(current_style);

    if (in.is_void()) {
        return;
    }

    auto const new_style = in.cast_to<uint64_t>();
    if (not new_style.has_value()) {
        throw abort_execution{ stack,
                               "invalid input operand for earithmeticstyle in "
                                   + op.instruction.in.to_string() };
    }

    auto const ns = static_cast<viua::vm::Process::Arithmetic_style>(
        static_cast<uint64_t>(*new_style));
    switch (ns) {
        using enum viua::vm::Process::Arithmetic_style;
        case Wrapping:
        case Trapping:
        case Saturating:
            stack.proc->arithmetic_style = ns;
            break;
        default:
            throw abort_execution{ stack,
                                   "unknown style for earithmeticstyle: "
                                       + std::to_string(
                                           static_cast<uint64_t>(*new_style)) };
    }
}

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

    auto const new_width = in.cast_to<uint64_t>();
    if (not new_width.has_value()) {
        throw abort_execution{ stack,
                               "invalid input operand for earithmeticwidth in "
                                   + op.instruction.in.to_string() };
    }

    stack.proc->arithmetic_width = static_cast<uint8_t>(*new_width);
}
}  // namespace viua::vm::ins
