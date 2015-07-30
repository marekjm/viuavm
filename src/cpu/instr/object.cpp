#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/object.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* CPU::vmnew(byte* addr) {
    /** Create new instance of specified class.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string class_name = string(addr);
    addr += (class_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new Object(class_name));

    return addr;
}
