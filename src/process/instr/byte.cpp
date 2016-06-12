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


byte* Process::opbstore(byte* addr) {
    unsigned destination_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    bool operand_ref = false;
    char operand;

    operand_ref = *(reinterpret_cast<bool*>(addr));
    pointer::inc<bool, byte>(addr);
    operand = static_cast<char>(*addr);
    ++addr;

    if (operand_ref) {
        operand = static_cast<Byte*>(fetch(static_cast<unsigned>(operand)))->value();
    }

    place(destination_register, new Byte(operand));

    return addr;
}
