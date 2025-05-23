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


#include <viua/arch/arch.h>
#include <viua/support/fdstream.h>
#include <viua/vm/backtrace.h>
#include <viua/vm/ins.h>


namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;

auto mutable_proxy(
    Stack& stack,
    access_type const a) -> Mutable_proxy
{
    if (not a.direct) {
        throw abort_execution{ stack, "dereferences are not implemented" };
    }

    switch (a.set) {
        using enum viua::arch::REGISTER_SET;
        case VOID:
            return { nullptr };
        case LOCAL:
            return { &stack.frames.back().registers.at(a.index) };
        case PARAMETER:
            return { &stack.frames.back().parameters.at(a.index) };
        case ARGUMENT:
            return { &stack.args.at(a.index) };
        default:
            throw abort_execution{
                stack, "illegal write access to register " + a.to_string()
            };
    }
}
auto immutable_proxy(
    Stack& stack,
    access_type const a) -> Immutable_proxy
{
    if (not a.direct) {
        throw abort_execution{ stack, "dereferences are not implemented" };
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
            throw abort_execution{
                stack, "illegal read access to register " + a.to_string()
            };
    }
}
auto immutable_proxy(
    Frame& frame,
    access_type const a,
    Stack const& stack) -> Immutable_proxy
{
    if (not a.direct) {
        throw abort_execution{ stack, "dereferences are not implemented" };
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
            throw abort_execution{
                stack, "illegal read access to register " + a.to_string()
            };
    }
}
}  // namespace viua::vm::ins
