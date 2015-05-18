#include <sstream>
#include <string>
#include "types/type.h"
#include "cpu/frame.h"
using namespace std;

string stringifyFunctionInvocation(const Frame* frame) {
    ostringstream oss;
    oss << frame->function_name << '/' << frame->args->size();
    oss << '(';
    for (unsigned i = 0; i < frame->args->size(); ++i) {
        Object* optr = frame->args->at(i);
        if (optr == 0) { continue; }
        oss << optr->repr();
        if (i < (frame->args->size()-1)) {
            oss << ", ";
        }
    }
    oss << ')';
    return oss.str();
}
