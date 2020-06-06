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

#include <chrono>
#include <memory>

#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/process.h>
#include <viua/types/boolean.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/process.h>
#include <viua/types/reference.h>
#include <viua/util/memory.h>


auto viua::process::Process::opprocess(Op_address_type addr) -> Op_address_type
{
    auto const target = decoder.fetch_register_or_void(addr, *this);

    auto call_name = std::string{};
    auto const ot  = viua::bytecode::codec::main::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        auto const fn =
            decoder.fetch_value_of<viua::types::Function>(addr, *this);

        call_name = fn->name();

        if (fn->type() == "Closure") {
            throw std::make_unique<viua::types::Exception>(
                "cannot spawn a process from closure");
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

    stack->frame_new->function_name = call_name;

    auto spawned_process = attached_scheduler->spawn(
        std::move(stack->frame_new), this, not target.has_value());
    if (target) {
        *target.value() =
            std::make_unique<viua::types::Process>(spawned_process);
    }

    return addr;
}
auto viua::process::Process::opjoin(Op_address_type addr) -> Op_address_type
{
    auto return_addr = Op_address_type{addr - 1};

    auto target     = decoder.fetch_register_or_void(addr, *this);
    auto const proc = decoder.fetch_value_of<viua::types::Process>(addr, *this);

    auto const timeout = decoder.fetch_timeout(addr);

    if (not attached_scheduler->is_joinable(proc->pid())) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Process_cannot_be_joined"}, proc->pid().str());
    }

    if (timeout and not timeout_active) {
        waiting_until  = (std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(timeout - 1));
        timeout_active = true;
    } else if (not timeout and not timeout_active) {
        wait_until_infinity = true;
        timeout_active      = true;
    }

    if (attached_scheduler->is_stopped(proc->pid())) {
        return_addr = addr;
        if (attached_scheduler->is_terminated(proc->pid())) {
            auto joined_throw =
                attached_scheduler->transfer_exception_of(proc->pid());

            /*
             * We need to detect if the joined throw value is an exception and,
             * if that is the case, rethrow it using exception type. This will
             * allow the process to add throw points to better track the
             * exception path.
             */
            if (auto joined_ex =
                    dynamic_cast<viua::types::Exception*>(joined_throw.get());
                joined_ex) {
                auto ex = std::unique_ptr<viua::types::Exception>();
                ex.reset(static_cast<viua::types::Exception*>(
                    joined_throw.release()));
                throw ex;
            } else {
                throw joined_throw;
            }
        } else {
            auto result = attached_scheduler->transfer_result_of(proc->pid());
            if (target) {
                *target.value() = std::move(result);
            }
        }
    } else if (timeout_active and (not wait_until_infinity)
               and (waiting_until < std::chrono::steady_clock::now())) {
        timeout_active      = false;
        wait_until_infinity = false;
        return_addr         = addr;
        throw std::make_unique<viua::types::Exception>("process did not join");
    }

    return return_addr;
}
auto viua::process::Process::opsend(Op_address_type addr) -> Op_address_type
{
    auto const proc = decoder.fetch_value_of<viua::types::Process>(addr, *this);
    auto source     = decoder.fetch_register(addr, *this);

    auto value = source->give();
    value->expire();
    attached_scheduler->send(proc->pid(), std::move(value));

    return addr;
}
auto viua::process::Process::opreceive(Op_address_type addr) -> Op_address_type
{
    auto return_addr = Op_address_type{addr - 1};

    auto target = decoder.fetch_register_or_void(addr, *this);

    auto const timeout           = decoder.fetch_timeout(addr);
    auto const immediate_timeout = (timeout == 1);

    if (immediate_timeout) {
        // do nothing
    } else if (timeout and not timeout_active) {
        waiting_until  = (std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(timeout - 1));
        timeout_active = true;
    } else if (not timeout and not timeout_active) {
        wait_until_infinity = true;
        timeout_active      = true;
    }

    attached_scheduler->receive(process_id, message_queue);

    if (not message_queue.empty()) {
        if (target) {
            *target.value() = std::move(message_queue.front());
        }
        message_queue.pop();
        timeout_active      = false;
        wait_until_infinity = false;
        return_addr         = addr;
    } else {
        auto const timeout_passed =
            (timeout_active and (not wait_until_infinity)
             and (waiting_until < std::chrono::steady_clock::now()));
        if (timeout_passed or immediate_timeout) {
            timeout_active      = false;
            wait_until_infinity = false;
            return_addr         = addr;
            throw std::make_unique<viua::types::Exception>(
                "no message received");
        }
    }

    return return_addr;
}
auto viua::process::Process::opwatchdog(Op_address_type addr) -> Op_address_type
{
    auto const call_name = decoder.fetch_string(addr);

    auto const is_native  = attached_scheduler->is_native_function(call_name);
    auto const is_foreign = attached_scheduler->is_foreign_function(call_name);

    if (not(is_native or is_foreign)) {
        throw std::make_unique<viua::types::Exception>(
            "watchdog process from undefined function: " + call_name);
    }
    if (not is_native) {
        throw std::make_unique<viua::types::Exception>(
            "watchdog process must be a native function, used foreign "
            + call_name);
    }

    if (not watchdog_function.empty()) {
        throw std::make_unique<viua::types::Exception>("watchdog already set");
    }

    watchdog_function = call_name;
    watchdog_frame    = std::move(stack->frame_new);

    return addr;
}
auto viua::process::Process::opself(Op_address_type addr) -> Op_address_type
{
    *decoder.fetch_register(addr, *this) =
        std::make_unique<viua::types::Process>(this);
    return addr;
}
auto viua::process::Process::oppideq(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);

    auto const rhs = decoder.fetch_value_of<viua::types::Process>(addr, *this);
    auto const lhs = decoder.fetch_value_of<viua::types::Process>(addr, *this);

    *target = std::make_unique<viua::types::Boolean>(*rhs == *lhs);

    return addr;
}
