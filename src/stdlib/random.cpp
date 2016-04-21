#include <cstdlib>
#include <climits>
#include <iostream>
#include <fstream>
#include <string>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


float getrandom() {
    /** Return random float between 0.0 and 1.0.
     *
     *  This is a utility function exposed to Viua.
     */
    ifstream in("/dev/urandom");
    if (!in) {
        throw new Exception("failed to open random device: /dev/urandom");
    }
    unsigned long long int rullint = 0;
    in.read((char*)&rullint, sizeof(unsigned long long int));
    float rfloat = ((long double)rullint / (long double)ULLONG_MAX);
    return rfloat;
}

void random_drandom(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    /** Return random integer.
     *
     *  Bytes are read from /dev/random random number device.
     *  This call can block if not enough entropy bytes are available.
     */
    ifstream in("/dev/random");
    if (!in) {
        throw new Exception("failed to open random device: /dev/random");
    }
    int rint = 0;
    in.read((char*)&rint, sizeof(int));
    frame->regset->set(0, new Integer(rint));
}

void random_durandom(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    /** Return random integer.
     *
     *  Bytes are read from /dev/urandom random number device.
     *  This is a nonblocking call.
     *  If not enough entropy bytes are available a pseudo-random generator is
     *  employed to supply missing bytes.
     */
    ifstream in("/dev/urandom");
    if (!in) {
        throw new Exception("failed to open random device: /dev/urandom");
    }
    int rint = 0;
    in.read((char*)&rint, sizeof(int));
    frame->regset->set(0, new Integer(rint));
}

void random_random(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    /** Return random float from range between 0.0 and 1.0.
     */
    frame->regset->set(0, new Float(getrandom()));
}

void random_randint(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    /** Return random integer from selected range.
     *
     *  Requires two parameters: lower and upper bound.
     *  Returned integer is in range [lower, upper).
     */
    int lower_bound = static_cast<Integer*>(frame->args->at(0))->value();
    int upper_bound = static_cast<Integer*>(frame->args->at(1))->value();
    int modifer = ((upper_bound - lower_bound) * getrandom());
    frame->regset->set(0, new Integer(lower_bound + modifer));
}

const ForeignFunctionSpec functions[] = {
    { "std::random::device::random", &random_drandom },
    { "std::random::device::urandom", &random_durandom },
    { "std::random::random", &random_random },
    { "std::random::randint", &random_randint },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
