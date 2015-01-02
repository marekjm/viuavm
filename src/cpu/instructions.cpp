#include <iostream>
#include <vector>
#include "../bytecode.h"
#include "../types/object.h"
#include "../types/integer.h"
#include "../types/boolean.h"
#include "../types/byte.h"
#include "../support/pointer.h"
#include "cpu.h"
using namespace std;


char* CPU::istore(char* addr) {
    /*  Run istore instruction.
     */
    int reg, num;
    bool reg_ref = false, num_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    reg = *((int*)addr);
    pointer::inc<int, char>(addr);

    num_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ISTORE";
        cout << (reg_ref ? " @" : " ") << reg;
        cout << (num_ref ? " @" : " ") << num;
        cout << endl;
    }

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }
    if (num_ref) {
        num = static_cast<Integer*>(fetch(num))->value();
    }

    registers[reg] = new Integer(num);

    return addr;
}

char* CPU::ilt(char* addr) {
    /*  Run ilt instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ILT";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    registers[regr_num] = new Boolean(rega_num < regb_num);

    return addr;
}

char* CPU::iinc(char* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regno = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IINC" << (ref ? " @" : " ") << regno;
    }

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    if (debug) {
        if (ref) { cout << " -> " << regno; }
        cout << endl;
    }

    ++(static_cast<Integer*>(fetch(regno))->value());

    return addr;
}

char* CPU::bstore(char* addr) {
    /*  Run bstore instruction.
     */
    int reg;
    bool reg_ref = false, byte_ref = false;
    byte bt;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    reg = *((int*)addr);
    pointer::inc<int, char>(addr);

    byte_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    bt = *((byte*)addr);
    ++addr;

    if (debug) {
        cout << "BSTORE";
        cout << (reg_ref ? " @" : " ") << reg;
        cout << (byte_ref ? " @" : " ");
        // this range is to display ASCII characters as their printable representations
        if (bt >= 32 and bt <= 127) {
            cout << '"' << bt << '"';
        } else {
            cout << (int)bt;
        }
        cout << endl;
    }

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }
    if (byte_ref) {
        bt = static_cast<Byte*>(fetch((int)bt))->value();
    }

    registers[reg] = new Byte(bt);

    return addr;
}

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
