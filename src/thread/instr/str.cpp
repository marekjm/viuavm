#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/cpu/opex.h>
#include <viua/support/string.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::strstore(byte* addr) {
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string svalue = string(addr);
    addr += svalue.size()+1;

    place(target, new String(str::strdecode(svalue)));

    return addr;
}
