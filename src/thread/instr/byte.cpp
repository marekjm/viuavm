#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/cpu/opex.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::bstore(byte* addr) {
    /*  Run bstore instruction.
     */
    int destination_register;
    bool destination_register_ref = false, operand_ref = false;
    byte operand;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register = *((int*)addr);
    pointer::inc<int, byte>(addr);

    operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    operand = *addr;
    ++addr;

    if (destination_register_ref) {
        destination_register = static_cast<Integer*>(fetch(destination_register))->value();
    }
    if (operand_ref) {
        operand = static_cast<Byte*>(fetch((int)operand))->value();
    }

    place(destination_register, new Byte(operand));

    return addr;
}
