#include <viua/cpu/frame.h>


void Frame::captureArguments() {
    deallocate_arguments = true;
    for (registerset_size_type i = 0; i < args->size(); ++i) {
        args->put(i, args->at(i)->copy());
    }
}

void Frame::setLocalRegisterSet(RegisterSet* rs, bool receives_ownership) {
    if (owns_local_register_set) {
        delete regset;
    }
    owns_local_register_set = receives_ownership;
    regset = rs;
}
