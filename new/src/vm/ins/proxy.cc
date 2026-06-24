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

#include <format>
#include <string>
#include <string_view>

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
    auto at_or_exception = [&stack, i = a.index](std::string_view const rs,
                                                 auto& v) -> Mutable_proxy
    {
        /*
         * Local register set always has 256 registers, but parameter and
         * argument sets are different. The capacity of the latter two sets is
         * determined by the "frame" instruction that allocated them. For
         * example:
         *
         *      frame $2.a
         *
         * allocates a frame with 2 (two) registers. From the caller's point of
         * view these are argument registers ie, they are supposed to be used to
         * pass arguments (or, actual parameters) to the callee; from the point
         * of view of the eventual callee, the same set will appear as parameter
         * registers ie, registers containing formal parameters of the function.
         *
         * The above fancy-sounding mumbo-jumbo is all fine and dandy, but what
         * it means is that both parameter and argument sets is dynamic, not
         * static, and thus require a dynamic check. Hence, the nice "if"
         * below this comment.
         */
        if (i >= v.size()) {
            /*
             * Whoopsie! This should not have happened.
             *
             * FIXME Improve static analysis run by the assembler to catch
             * out-of-bounds access to argument and parameter registers.
             */
            throw abort_execution{
                stack,
                std::format("access to nonexistent register ${}.{}", i, rs)
            };
        }
        return { &v.at(i) };
    };

    switch (a.set) {
        using enum viua::arch::REGISTER_SET;
        case SPECIAL:
            if (a.is_void()) {
                return { nullptr };
            } else {
                throw abort_execution{
                    stack, "illegal write access to register " + a.to_string()
                };
            }
        case LOCAL:
            return { &stack.frames.back().registers.at(a.index) };
        case PARAMETER:
            return at_or_exception("p", stack.frames.back().parameters);
        case ARGUMENT:
            return at_or_exception("a", stack.args);
        default:
            throw abort_execution{
                stack, "illegal write access to register " + a.to_string()
            };
    }
}
auto immutable_proxy(
    Stack const& stack,
    access_type const a) -> Immutable_proxy
{
    return immutable_proxy(stack.frames.back(), a, stack);
}
auto immutable_proxy(
    Frame const& frame,
    access_type const a,
    Stack const& stack) -> Immutable_proxy
{
    switch (a.set) {
        using enum viua::arch::REGISTER_SET;
        case SPECIAL:
            return immutable_proxy(
                stack, static_cast<viua::arch::SPECIAL_REGISTER>(a.index));
        case LOCAL:
            return frame.registers.at(a.index);
        case PARAMETER:
            return frame.parameters.at(a.index);
        case ARGUMENT:
            return stack.args.at(a.index);
    }
    throw abort_execution{ stack,
                           "illegal read access to register " + a.to_string() };
}
auto immutable_proxy(
    Stack const& stack,
    viua::arch::SPECIAL_REGISTER const r) -> Immutable_proxy
{
    static register_type const placeholder_void;
    static register_type const placeholder_zero_signed{
        register_type::int_type{}
    };
    static register_type const placeholder_zero_unsigned{
        register_type::uint_type{}
    };
    switch (r) {
        using enum viua::arch::SPECIAL_REGISTER;
        case VOID:
            return placeholder_void;
        case ZERO_SIGNED:
            return placeholder_zero_signed;
        case ZERO_UNSIGNED:
            return placeholder_zero_unsigned;
    }
    throw abort_execution{ stack,
                           "illegal special register "
                               + std::to_string(static_cast<unsigned int>(r)) };
}
}  // namespace viua::vm::ins
