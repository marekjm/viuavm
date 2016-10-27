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


string ProcessType::type() const {
    return "Process";
}

string ProcessType::str() const {
    return "Process";
}

string ProcessType::repr() const {
    return "Process";
}

bool ProcessType::boolean() const {
    return thrd->joinable();
}

ProcessType* ProcessType::copy() const {
    return new ProcessType(thrd);
}

bool ProcessType::joinable() {
    return thrd->joinable();
}

void ProcessType::join() {
    if (thrd->joinable()) {
        thrd->join();
    } else {
        throw new Exception("process cannot be joined");
    }
}

void ProcessType::detach() {
    if (thrd->joinable()) {
        thrd->detach();
    } else {
        throw new Exception("process cannot be detached");
    }
}

bool ProcessType::stopped() {
    return thrd->stopped();
}

bool ProcessType::terminated() {
    return thrd->terminated();
}

unique_ptr<Type> ProcessType::transferActiveException() {
    return thrd->transferActiveException();
}

unique_ptr<Type> ProcessType::getReturnValue() {
    return thrd->getReturnValue();
}


void ProcessType::joinable(Frame* frame, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    frame->regset->set(0, new Boolean(thrd->joinable()));
}

void ProcessType::detach(Frame*, RegisterSet*, RegisterSet*, Process*, Kernel*) {
    thrd->detach();
}


PID ProcessType::pid() const {
    return thrd->pid();
}
