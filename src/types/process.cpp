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
    return std::move(thrd->getReturnValue());
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
    frame->regset->set(0, new Integer(thrd->priority()));
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

    thrd->priority(static_cast<Integer*>(frame->args->at(1))->value());
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
