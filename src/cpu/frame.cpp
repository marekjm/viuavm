#include <viua/cpu/frame.h>


void Frame::setLocalRegisterSet(RegisterSet* rs, bool receives_ownership) {
    if (owns_local_register_set) {
        delete regset;
    }
    owns_local_register_set = receives_ownership;
    regset = rs;
}
