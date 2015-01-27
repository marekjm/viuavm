#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/float.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::fstore(byte* addr) {
    /*  Run istore instruction.
     */
    int reg;
    float value;
    bool ref = false;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    value = *((float*)addr);
    pointer::inc<float, byte>(addr);

    if (debug) {
        cout << (ref ? " @" : " ") << reg;
        cout << value;
    }

    if (ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new Float(value));

    return addr;
}
