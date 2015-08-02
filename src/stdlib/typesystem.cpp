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


Type* typeof(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }
    frame->regset->set(0, new String(frame->args->get(0)->type()));
    return 0;
}

Type* inheritanceChain(Frame* frame, RegisterSet*, RegisterSet*) {
    if (frame->args->at(0) == 0) {
        throw new Exception("expected object as parameter 0");
    }

    vector<string> ic = frame->args->at(0)->inheritancechain();
    Vector* icv = new Vector();

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(new String(ic[i]));
    }

    frame->regset->set(0, icv);
    return 0;
}

Type* bases(Frame* frame, RegisterSet*, RegisterSet*) {
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
    return 0;
}


const ExternalFunctionSpec functions[] = {
    { "typesystem::typeof", &typeof },
    { "typesystem::inheritanceChain", &inheritanceChain },
    { "typesystem::bases", &bases },
    { NULL, NULL },
};

extern "C" const ExternalFunctionSpec* exports() {
    return functions;
}
