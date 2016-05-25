#include <string>
#include <vector>
#include <viua/types/type.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/include/module.h>
using namespace std;


void typeof(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->type()));
}

void inheritanceChain(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }

    vector<string> ic = frame->args->at(0)->inheritancechain();
    Vector* icv = new Vector();

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(new String(ic[i]));
    }

    frame->regset->set(0, icv);
}

void bases(Frame* frame, RegisterSet*, RegisterSet*, Process*, CPU*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }

    Type* object = frame->args->at(0);
    vector<string> ic = object->bases();
    Vector* icv = new Vector();

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(new String(ic[i]));
    }

    frame->regset->set(0, icv);
}


const ForeignFunctionSpec functions[] = {
    { "typesystem::typeof/1", &typeof },
    { "typesystem::inheritanceChain/1", &inheritanceChain },
    { "typesystem::bases/1", &bases },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
