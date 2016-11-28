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


string viua::types::Process::type() const {
    return "Process";
}

string viua::types::Process::str() const {
    return "Process";
}

string viua::types::Process::repr() const {
    return "Process";
}

bool viua::types::Process::boolean() const {
    return thrd->joinable();
}

unique_ptr<viua::types::Type> viua::types::Process::copy() const {
    return unique_ptr<viua::types::Type>{new viua::types::Process(thrd)};
}

bool viua::types::Process::joinable() {
    return thrd->joinable();
}

void viua::types::Process::join() {
    if (thrd->joinable()) {
        thrd->join();
    } else {
        throw new viua::types::Exception("process cannot be joined");
    }
}

void viua::types::Process::detach() {
    if (thrd->joinable()) {
        thrd->detach();
    } else {
        throw new viua::types::Exception("process cannot be detached");
    }
}

bool viua::types::Process::stopped() {
    return thrd->stopped();
}

bool viua::types::Process::terminated() {
    return thrd->terminated();
}

unique_ptr<viua::types::Type> viua::types::Process::transferActiveException() {
    return thrd->transferActiveException();
}

unique_ptr<viua::types::Type> viua::types::Process::getReturnValue() {
    return thrd->getReturnValue();
}


void viua::types::Process::joinable(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    frame->regset->set(0, unique_ptr<viua::types::Type>{new viua::types::Boolean(thrd->joinable())});
}

void viua::types::Process::detach(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    thrd->detach();
}


viua::process::PID viua::types::Process::pid() const {
    return thrd->pid();
}
