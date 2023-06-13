/*
 *  Copyright (C) 2021-2023 Marek Marecki
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

auto execute(AA const op, Stack& stack, ip_type const) -> void
{
    auto const base = immutable_proxy(stack, op.instruction.in).get<uint64_t>();
    auto const alignment [[maybe_unused]] = (1u << op.instruction.spec);

    if (not base.has_value()) {
        throw abort_execution{stack, "invalid operand type for aa instruction"};
    }

    // FIXME Ensure that enough memory is available to satisfy both size and
    // alignment request.
    auto size = (*base * alignment);

    stack.proc->stack_break -= size;
    stack.frames.back().saved.sbrk = stack.proc->stack_break;
    auto const pointer_address     = stack.proc->stack_break;

    mutable_proxy(stack, op.instruction.out) =
        register_type::pointer_type{pointer_address};

    auto pointer_info = Pointer{};
    pointer_info.ptr  = pointer_address;
    pointer_info.size = size;
    stack.proc->record_pointer(pointer_info);
}
}  // namespace viua::vm::ins
