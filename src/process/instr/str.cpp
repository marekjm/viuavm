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


byte* Process::opstrstore(byte* addr) {
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    place(target, new String(str::strdecode(viua::operand::extractString(addr))));

    return addr;
}
