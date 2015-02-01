#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/float.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::itof(byte* addr) {
    /*  Run itof instruction.
     */
    bool rega_ref, regb_ref;
    int rega_num, regb_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }

    place(regb_num, new Float(static_cast<Integer*>(fetch(rega_num))->value()));

    return addr;
}

byte* CPU::ftoi(byte* addr) {
    /*  Run ftoi instruction.
     */
    bool rega_ref, regb_ref;
    int rega_num, regb_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }

    place(regb_num, new Integer(static_cast<Float*>(fetch(rega_num))->value()));

    return addr;
}
