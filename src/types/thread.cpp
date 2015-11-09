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


void ThreadType::joinable(Frame* frame, RegisterSet*, RegisterSet*) {
    frame->regset->set(0, new Boolean(thrd->joinable()));
}

void ThreadType::join(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() < 1) {
        throw new Exception("undefined this");
    }
}

void ThreadType::detach(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->size() < 1) {
        throw new Exception("undefined this");
    }
}
