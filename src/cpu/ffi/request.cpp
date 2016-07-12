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
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/include/module.h>
#include <viua/cpu/cpu.h>
using namespace std;

string ForeignFunctionCallRequest::functionName() const {
    return frame->function_name;
}
void ForeignFunctionCallRequest::call(ForeignFunction* callback) {
    /* FIXME: second parameter should be a pointer to static registers or
     *        0 if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     * FIXME: third parameter should be a pointer to global registers
     */
    try {
        (*callback)(frame, nullptr, nullptr, caller_process, cpu);

        /* // FIXME: woohoo! segfault! */
        Type* returned = nullptr;
        unsigned return_value_register = frame->place_return_value_in;
        bool resolve_return_value_register = frame->resolve_return_value_register;
        if (return_value_register != 0) {
            // we check in 0. register because it's reserved for return values
            if (frame->regset->at(0) == nullptr) {
                caller_process->raiseException(new Exception("return value requested by frame but external function did not set return register"));
            }
            returned = frame->regset->pop(0);
        }

        // place return value
        if (returned and caller_process->trace().size() > 0) {
            if (resolve_return_value_register) {
                return_value_register = static_cast<Integer*>(caller_process->obtain(return_value_register))->as_unsigned();
            }
            caller_process->put(return_value_register, returned);
        }
    } catch (Type *exception) {
        caller_process->raiseException(exception);
        caller_process->handleActiveException();
    }
}
void ForeignFunctionCallRequest::registerException(Type* object) {
    caller_process->raiseException(object);
}
void ForeignFunctionCallRequest::wakeup() {
    caller_process->wakeup();
}
