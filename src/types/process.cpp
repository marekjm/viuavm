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

Type* ProcessType::transferActiveException() {
    return thrd->transferActiveException();
}

unique_ptr<Type> ProcessType::getReturnValue() {
    return thrd->getReturnValue();
}


void ProcessType::joinable(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    frame->regset->set(0, new Boolean(thrd->joinable()));
}

void ProcessType::detach(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*) {
    thrd->detach();
}


void ProcessType::suspend(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*) {
    thrd->suspend();
}
void ProcessType::wakeup(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*) {
    thrd->wakeup();
}
void ProcessType::suspended(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    frame->regset->set(0, new Boolean(thrd->suspended()));
}


void ProcessType::getPriority(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    // FIXME: cast to silence compiler warning about implicit conversion changing signedness of the int
    // the cast is not *truly* safe, because unsigned can store positive numbers signed int cannot
    frame->regset->set(0, new Integer(static_cast<int>(thrd->priority())));
}

void ProcessType::setPriority(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    if (frame->args->at(0) == nullptr) {
        throw new Exception("expected Process as first parameter but got nothing");
    }
    if (frame->args->at(1) == nullptr) {
        throw new Exception("expected Integer as first parameter but got nothing");
    }
    if (frame->args->at(0)->type() != "Process") {
        throw new Exception("expected Process as first parameter but got " + frame->args->at(0)->type());
    }
    if (frame->args->at(1)->type() != "Integer") {
        throw new Exception("expected Integer as first parameter but got " + frame->args->at(0)->type());
    }

    int new_priority = static_cast<Integer*>(frame->args->at(1))->value();
    if (new_priority < 1) {
        throw new Exception("process priority must be a positive integer, got: " + frame->args->at(1)->str());
    }
    // FIXME: cast to silence compiler warning about implicit conversion changing signedness of the int
    // the cast is safe because the integer is guaranteed to be positive
    thrd->priority(static_cast<unsigned>(new_priority));
}

void ProcessType::pass(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    if (frame->args->at(0) == nullptr) {
        throw new Exception("expected Process as first parameter but got nothing");
    }
    if (frame->args->at(1) == nullptr) {
        throw new Exception("expected object as second parameter but got nothing");
    }
    if (frame->args->at(0)->type() != "Process") {
        throw new Exception("expected Process as first parameter but got " + frame->args->at(0)->type());
    }

    thrd->pass(unique_ptr<Type>(frame->args->at(1)->copy()));
}
