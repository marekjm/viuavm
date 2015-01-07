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
