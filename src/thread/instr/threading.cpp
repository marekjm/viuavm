#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::opthread(byte* addr) {
    /*  Run thread instruction.
     */
    string call_name = string(addr);

    bool is_native = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));
    bool is_foreign = cpu->foreign_functions.count(call_name);

    if (not (is_native or is_foreign)) {
        throw new Exception("call to undefined function: " + call_name);
    }

    auto caller = (is_native ? &Thread::callNative : &Thread::callForeign);
    return (this->*caller)(addr, call_name, false, 0, "");
}
