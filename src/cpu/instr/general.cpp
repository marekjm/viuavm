#include <iostream>
#include <vector>
#include "../../bytecode.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


char* CPU::echo(char* addr) {
    /*  Run echo instruction.
     */
    bool ref = false;
    int reg;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    reg = *((int*)addr);
    pointer::inc<int, char>(addr);

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

char* CPU::print(char* addr) {
    /*  Run print instruction.
     */
    addr = echo(addr);
    cout << '\n';
    return addr;
}

char* CPU::branch(char* addr) {
    /*  Run branch instruction.
     */
    if (debug) {
        cout << "BRANCH " << *(int*)addr << endl;
    }
    char* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw "aborting: BRANCH instruction pointing to itself";
    }
    return target;
}

char* CPU::branchif(char* addr) {
    /*  Run branchif instruction.
     */
    bool regcond_ref;
    int regcond_num;


    regcond_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regcond_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, char>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "BRANCHIF";
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
