#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::bstore(byte* addr) {
    unsigned destination_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    bool operand_ref = false;
    byte operand;

    operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    operand = *addr;
    ++addr;

    if (operand_ref) {
        operand = static_cast<Byte*>(fetch(static_cast<int>(operand)))->value();
    }

    place(destination_register, new Byte(operand));

    return addr;
}
