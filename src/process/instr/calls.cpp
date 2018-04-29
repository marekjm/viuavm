/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
using namespace std;


auto viua::process::Process::opframe(Op_address_type addr) -> Op_address_type {
    /** Create new frame for function calls.
     */
    viua::internals::types::register_index arguments = 0, local_registers = 0;
    tie(addr, arguments) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, local_registers) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    request_new_frame(arguments, local_registers);

    return addr;
}

auto viua::process::Process::opparam(Op_address_type addr) -> Op_address_type {
    /** Run param instruction.
     */
    viua::internals::types::register_index parameter_no_operand_index = 0;
    tie(addr, parameter_no_operand_index) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Value* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_object(addr, this);

    if (parameter_no_operand_index >= stack->frame_new->arguments->size()) {
        throw make_unique<viua::types::Exception>(
            "parameter register index out of bounds (greater than arguments "
            "set "
            "size) while adding parameter");
    }
    stack->frame_new->arguments->set(parameter_no_operand_index,
                                     source->copy());
    stack->frame_new->arguments->clear(parameter_no_operand_index);

    return addr;
}

auto viua::process::Process::oppamv(Op_address_type addr) -> Op_address_type {
    /** Run pamv instruction.
     */
    viua::internals::types::register_index parameter_no_operand_index = 0,
                                           source                     = 0;
    tie(addr, parameter_no_operand_index) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= stack->frame_new->arguments->size()) {
        throw make_unique<viua::types::Exception>(
            "parameter register index out of bounds (greater than arguments "
            "set "
            "size) while adding parameter");
    }
    stack->frame_new->arguments->set(parameter_no_operand_index,
                                     currently_used_register_set->pop(source));
    stack->frame_new->arguments->clear(parameter_no_operand_index);
    stack->frame_new->arguments->flag(parameter_no_operand_index, MOVED);

    return addr;
}

auto viua::process::Process::oparg(Op_address_type addr) -> Op_address_type {
    /** Run arg instruction.
     */
    viua::kernel::Register* target = nullptr;
    bool destination_is_void = viua::bytecode::decoder::operands::is_void(addr);

    if (not destination_is_void) {
        tie(addr, target) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::internals::types::register_index parameter_no_operand_index = 0;
    tie(addr, parameter_no_operand_index) =
        viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if (parameter_no_operand_index >= stack->back()->arguments->size()) {
        ostringstream oss;
        oss << "invalid read: read from argument register out of bounds: "
            << parameter_no_operand_index;
        throw make_unique<viua::types::Exception>(oss.str());
    }

    std::unique_ptr<viua::types::Value> argument;

    if (stack->back()->arguments->isflagged(parameter_no_operand_index,
                                            MOVED)) {
        argument = stack->back()->arguments->pop(parameter_no_operand_index);
    } else {
        argument =
            stack->back()->arguments->get(parameter_no_operand_index)->copy();
    }

    if (not destination_is_void) {
        *target = std::move(argument);
    }

    return addr;
}

auto viua::process::Process::opargc(Op_address_type addr) -> Op_address_type {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = make_unique<viua::types::Integer>(
        static_cast<int>(stack->back()->arguments->size()));

    return addr;
}

auto viua::process::Process::opcall(Op_address_type addr) -> Op_address_type {
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* return_register = nullptr;

    if (not return_void) {
        tie(addr, return_register) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    auto call_name = std::string{};
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Function* fn = nullptr;
        tie(addr, fn) = viua::bytecode::decoder::operands::fetch_object_of<
            viua::types::Function>(addr, this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->frame_new->set_local_register_set(
                static_cast<viua::types::Closure*>(fn)->rs(), false);
        }
    } else {
        tie(addr, call_name) =
            viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

    bool is_native  = scheduler->is_native_function(call_name);
    bool is_foreign = scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw make_unique<viua::types::Exception>("call to undefined function: "
                                                  + call_name);
    }

    auto caller = (is_native ? &viua::process::Process::call_native
                             : &viua::process::Process::call_foreign);
    return (this->*caller)(addr, call_name, return_register, "");
}

auto viua::process::Process::optailcall(Op_address_type addr)
    -> Op_address_type {
    if (stack->state_of() == viua::process::Stack::STATE::RUNNING) {
        stack->register_deferred_calls();
        stack->state_of(
            viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP);

        if (not stacks_order.empty()) {
            stack = stacks_order.top();
            stacks_order.pop();
            currently_used_register_set =
                stack->back()->local_register_set.get();
            return (addr - 1);
        }
    }

    if (stack->state_of()
        != viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
        throw make_unique<viua::types::Exception>(
            "stack left in an invalid state");
    }

    stack->state_of(viua::process::Stack::STATE::RUNNING);

    auto call_name = std::string{};
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Function* fn = nullptr;
        tie(addr, fn) = viua::bytecode::decoder::operands::fetch_object_of<
            viua::types::Function>(addr, this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->back()->local_register_set.reset(
                static_cast<viua::types::Closure*>(fn)->give());
            currently_used_register_set =
                stack->back()->local_register_set.get();
        }
    } else {
        tie(addr, call_name) =
            viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

    bool is_native  = scheduler->is_native_function(call_name);
    bool is_foreign = scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw make_unique<viua::types::Exception>(
            "tail call to undefined function: " + call_name);
    }
    // FIXME: make to possible to tail call foreign functions and methods
    if (not is_native) {
        throw make_unique<viua::types::Exception>(
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

auto viua::process::Process::opdefer(Op_address_type addr) -> Op_address_type {
    auto call_name = std::string{};
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Function* fn = nullptr;
        tie(addr, fn) = viua::bytecode::decoder::operands::fetch_object_of<
            viua::types::Function>(addr, this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            stack->back()->local_register_set.reset(
                static_cast<viua::types::Closure*>(fn)->give());
            currently_used_register_set =
                stack->back()->local_register_set.get();
        }
    } else {
        tie(addr, call_name) =
            viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

    bool is_native  = scheduler->is_native_function(call_name);
    bool is_foreign = scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw make_unique<viua::types::Exception>(
            "defer of undefined function: " + call_name);
    }

    push_deferred(call_name);

    return addr;
}

auto viua::process::Process::opreturn(Op_address_type addr) -> Op_address_type {
    if (stack->size() == 0) {
        throw make_unique<viua::types::Exception>(
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
            currently_used_register_set =
                stack->back()->local_register_set.get();
            return addr;
        }
    }

    if (stack->state_of()
        != viua::process::Stack::STATE::SUSPENDED_BY_DEFERRED_ON_FRAME_POP) {
        throw make_unique<viua::types::Exception>(
            "stack left in an invalid state");
    }

    addr = stack->back()->ret_address();

    std::unique_ptr<viua::types::Value> returned;
    viua::kernel::Register* return_register = stack->back()->return_register;
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
