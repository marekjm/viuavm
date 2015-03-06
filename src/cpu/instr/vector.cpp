#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/float.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::vec(byte* addr) {
    /*  Run vec instruction.
     */
    bool reg_num;
    int reg_num;

    reg_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (reg_num ? " @" : " ") << reg_num;
    }

    if (reg_num) {
        if (debug) { cout << "resolving numerence to operand register" << endl; }
        reg_num = static_cast<Integer*>(registers[reg_num])->value();
    }

    place(reg_num, new Vector());

    return addr;
}

byte* CPU::vinsert(byte* addr) {
    /*  Run vinsert instruction.
     *
     *  Vector always inserts a copy of the object in a register.
     *  FIXME: make it possible to insert references.
     */
    bool regvec_num, regval_num, regpos_num;
    int regvec_num, regval_num, regpos_num;

    regvec_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regval_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regval_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regpos_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regpos_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_num ? " @" : " ") << regvec_num;
        cout << (regval_num ? " @" : " ") << regval_num;
        cout << (regpos_num ? " @" : " ") << regval_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving numerence to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(registers[regvec_num])->value();
    }
    if (regval_ref) {
        if (debug) { cout << "resolving numerence to 2-operand register" << endl; }
        regval_num = static_cast<Integer*>(registers[regval_num])->value();
    }
    if (regpos_ref) {
        if (debug) { cout << "resolving numerence to 3-operand register" << endl; }
        regpos_num = static_cast<Integer*>(registers[regpos_num])->value();
    }

    fetch(regvec_num)->insert(regpos_num, fetch(regval_num)->copy());

    return addr;
}

byte* CPU::vpush(byte* addr) {
    /*  Run vpush instruction.
     *
     *  Vector always pushes a copy of the object in a register.
     *  FIXME: make it possible to push references.
     */
    bool regvec_num, regval_num;
    int regvec_num, regval_num;

    regvec_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regval_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regval_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_num ? " @" : " ") << regvec_num;
        cout << (regval_num ? " @" : " ") << regval_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving numerence to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(registers[regvec_num])->value();
    }
    if (regval_ref) {
        if (debug) { cout << "resolving numerence to 2-operand register" << endl; }
        regval_num = static_cast<Integer*>(registers[regval_num])->value();
    }

    fetch(regvec_num)->push(fetch(regval_num)->copy());

    return addr;
}

byte* CPU::vpop(byte* addr) {
    /*  Run vpop instruction.
     *
     *  Vector always pops a copy of the object in a register.
     *  FIXME: make it possible to pop references.
     */
    bool regvec_num, regdst_num, regpos_num;
    int regvec_num, regdst_num, regpos_num;

    regvec_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regdst_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regdst_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regpos_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regpos_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_num ? " @" : " ") << regvec_num;
        cout << (regdst_num ? " @" : " ") << regdst_num;
        cout << (regpos_num ? " @" : " ") << regdst_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving numerence to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(registers[regvec_num])->dstue();
    }
    if (regdst_ref) {
        if (debug) { cout << "resolving numerence to 2-operand register" << endl; }
        regdst_num = static_cast<Integer*>(registers[regdst_num])->dstue();
    }
    if (regpos_ref) {
        if (debug) { cout << "resolving numerence to 3-operand register" << endl; }
        regpos_num = static_cast<Integer*>(registers[regpos_num])->dstue();
    }

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Object* ptr = fetch(regvec_num)->pop(regpos_num);
    if (regdst_num) { place(regdst_num, ptr); }

    return addr;
}

byte* CPU::vlen(byte* addr) {
    /*  Run vlen instruction.
     */
    bool regvec_num, regval_num;
    int regvec_num, regval_num;

    regvec_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regval_num = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regval_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_num ? " @" : " ") << regvec_num;
        cout << (regval_num ? " @" : " ") << regval_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving numerence to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(registers[regvec_num])->value();
    }
    if (regval_ref) {
        if (debug) { cout << "resolving numerence to 2-operand register" << endl; }
        regval_num = static_cast<Integer*>(registers[regval_num])->value();
    }

    place(regval_num, new Integer(fetch(regvec_num)->len()));

    return addr;
}
