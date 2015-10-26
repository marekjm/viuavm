#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::strstore(byte* addr) {
    /*  Run strstore instruction.
     */
    int reg;
    bool reg_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string svalue = string(addr);
    addr += svalue.size()+1;

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new String(str::strdecode(svalue)));

    return addr;
}
