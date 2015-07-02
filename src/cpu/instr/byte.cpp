#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::bstore(byte* addr) {
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
    operand = *((byte*)addr);
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
