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
    int reg;
    bool reg_ref = false, byte_ref = false;
    byte bt;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    byte_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    bt = *((byte*)addr);
    ++addr;

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }
    if (byte_ref) {
        bt = static_cast<Byte*>(fetch((int)bt))->value();
    }

    place(reg, new Byte(bt));

    return addr;
}
