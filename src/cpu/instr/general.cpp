#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::echo(byte* addr) {
    /*  Run echo instruction.
     */
    bool ref = false;
    int reg;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "ECHO" << (ref ? " @" : " ") << reg;
        cout << endl;
    }

    if (ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    cout << fetch(reg)->str();

    if (debug) { cout << endl; }

    return addr;
}

byte* CPU::print(byte* addr) {
    /*  Run print instruction.
     */
    if (debug) {
        cout << "PRINT -> ";
    }
    addr = echo(addr);
    cout << '\n';
    return addr;
}

byte* CPU::jump(byte* addr) {
    /*  Run jump instruction.
     */
    if (debug) {
        cout << "JUMP " << *(int*)addr << endl;
    }
    byte* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw "aborting: JUMP instruction pointing to itself";
    }
    return target;
}

byte* CPU::branch(byte* addr) {
    /*  Run branch instruction.
     */
    bool regcond_ref;
    int regcond_num;


    regcond_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regcond_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << "BRANCH";
        cout << dec << (regcond_ref ? " @" : " ") << regcond_num;
        cout << " " << addr_true  << "::0x" << hex << (long)(bytecode+addr_true) << dec;
        cout << " " << addr_false << "::0x" << hex << (long)(bytecode+addr_false);
        cout << endl;
    }


    if (regcond_ref) {
        if (debug) { cout << "resolving reference to condition register" << endl; }
        regcond_num = static_cast<Integer*>(fetch(regcond_num))->value();
    }

    bool result = fetch(regcond_num)->boolean();

    addr = bytecode + (result ? addr_true : addr_false);

    return addr;
}
