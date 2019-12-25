/*
 *  Copyright (C) 2019, 2020 Marek Marecki
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

#include <iostream>
#include <memory>
#include <unistd.h>
#include <viua/process.h>
#include <viua/scheduler/process.h>
#include <viua/types/integer.h>
#include <viua/types/io.h>
#include <viua/types/string.h>
#include <viua/types/value.h>

auto viua::process::Process::op_io_read(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto porty  = decoder.fetch_register(addr, *this);
    auto limity = decoder.fetch_register(addr, *this);

    if (dynamic_cast<viua::types::Integer*>(porty->get())) {
        auto port = std::make_unique<viua::types::IO_fd>(
            static_cast<viua::types::Integer&>(*porty->get()).as_integer(),
            viua::types::IO_fd::Ownership::Borrowed);
        *target = port->read(attached_scheduler->kernel(), limity->give());
    } else if (dynamic_cast<viua::types::IO_fd*>(porty->get())) {
        auto& port = static_cast<viua::types::IO_fd&>(*porty->get());
        *target    = port.read(attached_scheduler->kernel(), limity->give());
    }

    return addr;
}

auto viua::process::Process::op_io_write(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto porty  = decoder.fetch_register(addr, *this);
    auto data   = decoder.fetch_register(addr, *this);

    if (dynamic_cast<viua::types::Integer*>(porty->get())) {
        auto port = std::make_unique<viua::types::IO_fd>(
            static_cast<viua::types::Integer&>(*porty->get()).as_integer(),
            viua::types::IO_fd::Ownership::Borrowed);
        *target = port->write(attached_scheduler->kernel(), data->give());
    } else if (dynamic_cast<viua::types::IO_fd*>(porty->get())) {
        auto& port = static_cast<viua::types::IO_fd&>(*porty->get());
        *target    = port.write(attached_scheduler->kernel(), data->give());
    }

    return addr;
}

auto viua::process::Process::op_io_close(Op_address_type addr)
    -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);
    auto port   = decoder.fetch_value_of<viua::types::IO_port>(addr, *this);

    *target = port->close(attached_scheduler->kernel());

    return addr;
}

auto viua::process::Process::op_io_wait(Op_address_type addr) -> Op_address_type
{
    /*
     * Wait for an I/O request to complete or a timeout to pass. If the request
     * completes in the specified time its result is returned in the destination
     * register; otherwise an exception is thrown.
     */

    /*
     * By default return to just before the instruction. This is the same dumb
     * and inefficient solution that is used to implement timeouts for the
     * receive instruction. However, this avoids global synchronisation so may
     * not be that bad after all.
     */
    auto return_addr = Op_address_type{addr - 1};

    auto target  = decoder.fetch_register_or_void(addr, *this);
    auto request = decoder.fetch_value_of<viua::types::IO_request>(addr, *this);
    auto const timeout = decoder.fetch_timeout(addr);

    if (timeout and not timeout_active) {
        waiting_until  = (std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(timeout - 1));
        timeout_active = true;
    } else if (not timeout and not timeout_active) {
        wait_until_infinity = true;
        timeout_active      = true;
    }

    if (attached_scheduler->io_complete(request->id())) {
        auto result = attached_scheduler->io_result(request->id());
        if (target) {
            **target = std::move(result);
        }

        timeout_active      = false;
        wait_until_infinity = false;
        return_addr         = addr;
    } else {
        if (timeout_active and (not wait_until_infinity)
            and (waiting_until < std::chrono::steady_clock::now())) {
            timeout_active      = false;
            wait_until_infinity = false;
            stack->thrown =
                std::make_unique<viua::types::Exception>("I/O not completed");
            return_addr = addr;
        }
    }

    return return_addr;
}

auto viua::process::Process::op_io_cancel(Op_address_type addr)
    -> Op_address_type
{
    cancel_io(
        decoder.fetch_value_of<viua::types::IO_request>(addr, *this)->id());

    return addr;
}
