#include <viua/cpu/frame.h>


void Frame::captureArguments() {
    deallocate_arguments = true;
    for (registerset_size_type i = 0; i < args->size(); ++i) {
        args->put(i, args->at(i)->copy());
    }
}
