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

#include <iostream>
#include <string>
#include <viua/types/boolean.h>
#include <viua/types/process.h>
#include <viua/exceptions.h>
#include <viua/process.h>
#include <viua/kernel/kernel.h>
using namespace std;


string viua::types::ProcessType::type() const {
    return "Process";
}

string viua::types::ProcessType::str() const {
    return "Process";
}

string viua::types::ProcessType::repr() const {
    return "Process";
}

bool viua::types::ProcessType::boolean() const {
    return thrd->joinable();
}

viua::types::ProcessType* viua::types::ProcessType::copy() const {
    return new viua::types::ProcessType(thrd);
}

bool viua::types::ProcessType::joinable() {
    return thrd->joinable();
}

void viua::types::ProcessType::join() {
    if (thrd->joinable()) {
        thrd->join();
    } else {
        throw new viua::types::Exception("process cannot be joined");
    }
}

void viua::types::ProcessType::detach() {
    if (thrd->joinable()) {
        thrd->detach();
    } else {
        throw new viua::types::Exception("process cannot be detached");
    }
}

bool viua::types::ProcessType::stopped() {
    return thrd->stopped();
}

bool viua::types::ProcessType::terminated() {
    return thrd->terminated();
}

unique_ptr<viua::types::Type> viua::types::ProcessType::transferActiveException() {
    return thrd->transferActiveException();
}

unique_ptr<viua::types::Type> viua::types::ProcessType::getReturnValue() {
    return thrd->getReturnValue();
}


void viua::types::ProcessType::joinable(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    frame->regset->set(0, new viua::types::Boolean(thrd->joinable()));
}

void viua::types::ProcessType::detach(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    thrd->detach();
}


PID viua::types::ProcessType::pid() const {
    return thrd->pid();
}
