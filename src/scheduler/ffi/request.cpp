/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#include <string>

#include <viua/include/module.h>
#include <viua/process.h>
#include <viua/scheduler/ffi.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>


auto viua::scheduler::ffi::Foreign_function_call_request::function_name() const
    -> std::string
{
    return frame->function_name;
}
auto viua::scheduler::ffi::Foreign_function_call_request::call(
    ForeignFunction* callback) -> void
{
    /* FIXME: second parameter should be a pointer to static registers or
     *        nullptr if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     * FIXME: third parameter should be a pointer to global registers
     */
    try {
        (*callback)(frame.get(), nullptr, nullptr, &caller_process, &kernel);

        std::unique_ptr<viua::types::Value> returned;
        auto return_register = frame->return_register;
        if (return_register != nullptr) {
            // we check in 0. register because it's reserved for return values
            if (frame->local_register_set->at(0) == nullptr) {
                throw std::make_unique<viua::types::Exception>(
                    "return value requested by frame but external function did "
                    "not set return register");
            }
            returned = frame->local_register_set->pop(0);
        }

        // place return value
        if (returned and caller_process.depth() > 0) {
            *return_register = std::move(returned);
        }
    } catch (std::unique_ptr<viua::types::Exception>& exception) {
        exception->add_throw_point(
            viua::types::Exception::Throw_point{function_name()});
        caller_process.raise(std::move(exception));
        caller_process.handle_active_exception();
    } catch (std::unique_ptr<viua::types::Value>& value) {
        using viua::types::Exception;
        auto exception = std::make_unique<Exception>(std::move(value));
        exception->add_throw_point(Exception::Throw_point{function_name()});
        caller_process.raise(std::move(exception));
        caller_process.handle_active_exception();
    }
}
auto viua::scheduler::ffi::Foreign_function_call_request::raise(
    std::unique_ptr<viua::types::Value> object) -> void
{
    caller_process.raise(std::move(object));
}
auto viua::scheduler::ffi::Foreign_function_call_request::wakeup() -> void
{
    caller_process.wakeup();
}
