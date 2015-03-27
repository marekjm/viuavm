#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../types/boolean.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::lognot(byte* addr) {
    /*  Run idec instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regno = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    place(regno, new Boolean(not fetch(regno)->boolean()));

    return addr;
}

byte* CPU::logand(byte* addr) {
    /*  Run ieq instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (rega_ref) {
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    place(regr_num, new Boolean(fetch(rega_num)->boolean() and fetch(regb_num)->boolean()));

    return addr;
}

byte* CPU::logor(byte* addr) {
    /*  Run ieq instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (rega_ref) {
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    place(regr_num, new Boolean(fetch(rega_num)->boolean() or fetch(regb_num)->boolean()));

    return addr;
}
