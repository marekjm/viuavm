/*
 *  Copyright (C) 2015-2020 Marek Marecki
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

#include <memory>

#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/process.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>

auto viua::process::Process::opframe(Op_address_type addr) -> Op_address_type
{
    /** Create new frame for function calls.
     */
    auto const arguments = decoder.fetch_register_index(addr);

    request_new_frame(arguments);

    return addr;
}

auto viua::process::Process::opparam(Op_address_type addr) -> Op_address_type
{
    /** Run param instruction.
     */
    auto const parameter_no_operand_index = decoder.fetch_register_index(addr);
    auto const source                     = decoder.fetch_value(addr, *this);

    if (parameter_no_operand_index >= stack->frame_new->arguments->size()) {
        throw std::make_unique<viua::types::Exception>(
            "parameter register index out of bounds (greater than arguments "
            "set "
            "size) while adding parameter");
    }
    stack->frame_new->arguments->set(parameter_no_operand_index,
                                     source->copy());
    stack->frame_new->arguments->clear(parameter_no_operand_index);

    return addr;
}

auto viua::process::Process::oppamv(Op_address_type addr) -> Op_address_type
{
    /** Run pamv instruction.
     */
    auto const parameter_no_operand_index = decoder.fetch_register_index(addr);
    auto const source                     = decoder.fetch_register(addr, *this);

    if (parameter_no_operand_index >= stack->frame_new->arguments->size()) {
        throw std::make_unique<viua::types::Exception>(
            "parameter register index out of bounds (greater than arguments "
            "set "
            "size) while adding parameter");
    }
    stack->frame_new->arguments->set(parameter_no_operand_index,
                                     source->give());
    stack->frame_new->arguments->clear(parameter_no_operand_index);
    stack->frame_new->arguments->flag(parameter_no_operand_index, MOVED);

    return addr;
}

auto viua::process::Process::oparg(Op_address_type addr) -> Op_address_type
{
    /** Run arg instruction.
     */
    auto const target = decoder.fetch_register_or_void(addr, *this);
    auto const parameter_no_operand_index = decoder.fetch_register_index(addr);

    if (parameter_no_operand_index >= stack->back()->arguments->size()) {
        auto oss = std::ostringstream{};
        oss << "invalid read: read from argument register out of bounds: "
            << parameter_no_operand_index;
        throw std::make_unique<viua::types::Exception>(oss.str());
    }

    auto argument = stack->back()->arguments->pop(parameter_no_operand_index);

    if (target) {
        *target.value() = std::move(argument);
    }

    return addr;
}

auto viua::process::Process::opallocate_registers(Op_address_type addr)
    -> Op_address_type
{
    auto const no_of_registers = decoder.fetch_register_index(addr);

    auto allocated =
        std::make_unique<viua::kernel::Register_set>(no_of_registers);
    stack->back()->set_local_register_set(std::move(allocated));

    return addr;
}

auto viua::process::Process::opcall(Op_address_type addr) -> Op_address_type
{
    auto const return_register = decoder.fetch_register_or_void(addr, *this);

    auto call_name = std::string{};
    auto ot        = viua::bytecode::codec::main::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn =
            decoder.fetch_value_of<viua::types::Function>(addr, *this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->frame_new->set_local_register_set(
                static_cast<viua::types::Closure*>(fn)->rs(), false);
        }
    } else {
        call_name = decoder.fetch_string(addr);
    }

    auto const is_native  = attached_scheduler->is_native_function(call_name);
    auto const is_foreign = attached_scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw std::make_unique<viua::types::Exception>(
            "call to undefined function: " + call_name);
    }

    auto caller = (is_native ? &viua::process::Process::call_native
                             : &viua::process::Process::call_foreign);
    return (this->*caller)(
        addr, call_name, return_register.value_or(nullptr), "");
}

auto viua::process::Process::optailcall(Op_address_type addr) -> Op_address_type
{
    if (stack->state_of() == viua::process::Stack::STATE::RUNNING) {
        stack->register_deferred_calls();
        stack->state_of(
            viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP);

        if (not stacks_order.empty()) {
            stack = stacks_order.top();
            stacks_order.pop();
            return (addr - 1);
        }
    }

    if (stack->state_of()
        != viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
        throw std::make_unique<viua::types::Exception>(
            "stack left in an invalid state");
    }

    stack->state_of(viua::process::Stack::STATE::RUNNING);

    auto call_name = std::string{};
    auto ot        = viua::bytecode::codec::main::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn =
            decoder.fetch_value_of<viua::types::Function>(addr, *this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->back()->local_register_set.reset(
                static_cast<viua::types::Closure*>(fn)->give());
        }
    } else {
        call_name = decoder.fetch_string(addr);
    }

    auto const is_native  = attached_scheduler->is_native_function(call_name);
    auto const is_foreign = attached_scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw std::make_unique<viua::types::Exception>(
            "tail call to undefined function: " + call_name);
    }
    // FIXME: make to possible to tail call foreign functions and methods
    if (not is_native) {
        throw std::make_unique<viua::types::Exception>(
            "tail call to non-native function: " + call_name);
    }

    // FIXME tailcalled functions should not inherit local register set of the
    // frame they replace
    stack->back()->arguments = std::move(stack->frame_new->arguments);

    // new frame must be deleted to prevent future errors
    // it's a simulated "push-and-pop" from the stack
    stack->frame_new.reset(nullptr);

    return adjust_jump_base_for(call_name);
}

auto viua::process::Process::opdefer(Op_address_type addr) -> Op_address_type
{
    auto call_name = std::string{};
    auto ot        = viua::bytecode::codec::main::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn =
            decoder.fetch_value_of<viua::types::Function>(addr, *this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->back()->local_register_set.reset(
                static_cast<viua::types::Closure*>(fn)->give());
        }
    } else {
        call_name = decoder.fetch_string(addr);
    }

    auto const is_native  = attached_scheduler->is_native_function(call_name);
    auto const is_foreign = attached_scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw std::make_unique<viua::types::Exception>(
            "defer of undefined function: " + call_name);
    }

    push_deferred(call_name);

    return addr;
}

auto viua::process::Process::opreturn(Op_address_type addr) -> Op_address_type
{
    if (stack->size() == 0) {
        throw std::make_unique<viua::types::Exception>(
            "no frame on stack: no call to return from");
    }

    if (stack->state_of() == viua::process::Stack::STATE::RUNNING) {
        stack->set_return_value();
        stack->register_deferred_calls();
        stack->state_of(
            viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP);

        if (not stacks_order.empty()) {
            stack = stacks_order.top();
            stacks_order.pop();
            return addr;
        }
    }

    if (stack->state_of()
        != viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
        throw std::make_unique<viua::types::Exception>(
            "stack left in an invalid state");
    }

    addr = stack->back()->ret_address();

    std::unique_ptr<viua::types::Value> returned;
    auto return_register = stack->back()->return_register;
    if (return_register != nullptr) {
        returned = std::move(stack->return_value);
    }

    stack->state_of(viua::process::Stack::STATE::RUNNING);

    stack->pop();

    // place return value
    if (returned and stack->size() > 0) {
        *return_register = std::move(returned);
    }

    if (stack->size() > 0) {
        adjust_jump_base_for(stack->back()->function_name);
    }

    return addr;
}
