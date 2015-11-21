#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/cpu/opex.h>
#include <viua/support/string.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::strstore(byte* addr) {
    /*  Run strstore instruction.
     */
    int reg;
    bool reg_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, reg_ref, reg);

    string svalue = string(addr);
    addr += svalue.size()+1;

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new String(str::strdecode(svalue)));

    return addr;
}
