#include <iostream>
#include <string>
#include <viua/types/boolean.h>
#include <viua/types/thread.h>
#include <viua/exceptions.h>
#include <viua/thread.h>
using namespace std;


string ThreadType::type() const {
    return "Thread";
}

string ThreadType::str() const {
    return "Thread";
}

string ThreadType::repr() const {
    return "Thread";
}

bool ThreadType::boolean() const {
    return thrd->joinable();
}

ThreadType* ThreadType::copy() const {
    return new ThreadType(thrd);
}

bool ThreadType::joinable() {
    return thrd->joinable();
}

void ThreadType::join() {
    if (thrd->joinable()) {
        thrd->join();
    } else {
        throw new Exception("thread cannot be joined");
    }
}

void ThreadType::detach() {
    if (thrd->joinable()) {
        thrd->detach();
    } else {
        throw new Exception("thread cannot be detached");
    }
}

bool ThreadType::stopped() {
    return thrd->stopped();
}

void ThreadType::joinable(Frame* frame, RegisterSet*, RegisterSet*) {
    frame->regset->set(0, new Boolean(thrd->joinable()));
}

void ThreadType::detach(Frame* frame, RegisterSet*, RegisterSet*) {
    thrd->detach();
}

void ThreadType::getPriority(Frame* frame, RegisterSet*, RegisterSet*) {
    frame->regset->set(0, new Integer(thrd->priority()));
}

void ThreadType::setPriority(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == nullptr) {
        throw new Exception("expected Thread as first parameter but got nothing");
    }
    if (frame->args->at(1) == nullptr) {
        throw new Exception("expected Integer as first parameter but got nothing");
    }
    if (frame->args->at(0)->type() != "Thread") {
        throw new Exception("expected Thread as first parameter but got " + frame->args->at(0)->type());
    }
    if (frame->args->at(1)->type() != "Integer") {
        throw new Exception("expected Integer as first parameter but got " + frame->args->at(0)->type());
    }

    thrd->priority(static_cast<Integer*>(frame->args->at(1))->value());
}
