#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/vector.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::vec(byte* addr) {
    /*  Run vec instruction.
     */
    bool reg_ref;
    int reg_num;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (reg_ref ? " @" : " ") << reg_num;
    }

    if (reg_ref) {
        if (debug) { cout << "resolving reference to operand register" << endl; }
        reg_num = static_cast<Integer*>(uregisters[reg_num])->value();
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
    bool regvec_ref, regval_ref, regpos_ref;
    int regvec_num, regval_num, regpos_num;

    regvec_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regval_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regval_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regpos_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regpos_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_ref ? " @" : " ") << regvec_num;
        cout << (regval_ref ? " @" : " ") << regval_num;
        cout << (regpos_ref ? " @" : " ") << regpos_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving reference to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(uregisters[regvec_num])->value();
    }
    if (regval_ref) {
        if (debug) { cout << "resolving reference to 2-operand register" << endl; }
        regval_num = static_cast<Integer*>(uregisters[regval_num])->value();
    }
    if (regpos_ref) {
        if (debug) { cout << "resolving reference to 3-operand register" << endl; }
        regpos_num = static_cast<Integer*>(uregisters[regpos_num])->value();
    }

    static_cast<Vector*>(fetch(regvec_num))->insert(regpos_num, fetch(regval_num)->copy());

    return addr;
}

byte* CPU::vpush(byte* addr) {
    /*  Run vpush instruction.
     *
     *  Vector always pushes a copy of the object in a register.
     *  FIXME: make it possible to push references.
     */
    bool regvec_ref, regval_ref;
    int regvec_num, regval_num;

    regvec_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regval_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regval_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_ref ? " @" : " ") << regvec_num;
        cout << (regval_ref ? " @" : " ") << regval_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving reference to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(uregisters[regvec_num])->value();
    }
    if (regval_ref) {
        if (debug) { cout << "resolving reference to 2-operand register" << endl; }
        regval_num = static_cast<Integer*>(uregisters[regval_num])->value();
    }

    static_cast<Vector*>(fetch(regvec_num))->push(fetch(regval_num)->copy());

    return addr;
}

byte* CPU::vpop(byte* addr) {
    /*  Run vpop instruction.
     *
     *  Vector always pops a copy of the object in a register.
     *  FIXME: make it possible to pop references.
     */
    bool regvec_ref, regdst_ref, regpos_ref;
    int regvec_num, regdst_num, regpos_num;

    regvec_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regdst_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regdst_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regpos_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regpos_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_ref ? " @" : " ") << regvec_num;
        cout << (regdst_ref ? " @" : " ") << regdst_num;
        cout << (regpos_ref ? " @" : " ") << regpos_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving reference to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(uregisters[regvec_num])->value();
    }
    if (regdst_ref) {
        if (debug) { cout << "resolving reference to 2-operand register" << endl; }
        regdst_num = static_cast<Integer*>(uregisters[regdst_num])->value();
    }
    if (regpos_ref) {
        if (debug) { cout << "resolving reference to 3-operand register" << endl; }
        regpos_num = static_cast<Integer*>(uregisters[regpos_num])->value();
    }

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Object* ptr = static_cast<Vector*>(fetch(regvec_num))->pop(regpos_num);
    if (regdst_num) { place(regdst_num, ptr); }

    return addr;
}

byte* CPU::vat(byte* addr) {
    /*  Run vat instruction.
     *
     *  Vector always returns a copy of the object in a register.
     *  FIXME: make it possible to pop references.
     */
    bool regvec_ref, regdst_ref, regpos_ref;
    int regvec_num, regdst_num, regpos_num;

    regvec_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regdst_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regdst_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regpos_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regpos_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_ref ? " @" : " ") << regvec_num;
        cout << (regdst_ref ? " @" : " ") << regdst_num;
        cout << (regpos_ref ? " @" : " ") << regpos_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "\nresolving reference to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(fetch(regvec_num))->value();
    }
    if (regdst_ref) {
        if (debug) { cout << "\nresolving reference to 2-operand register" << endl; }
        regdst_num = static_cast<Integer*>(uregisters[regdst_num])->value();
    }
    if (regpos_ref) {
        if (debug) { cout << "\nresolving reference to 3-operand register" << endl; }
        regpos_num = static_cast<Integer*>(uregisters[regpos_num])->value();
    }

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Object* ptr = static_cast<Vector*>(fetch(regvec_num))->at(regpos_num);
    if (regdst_num) {
        place(regdst_num, ptr);
        ureferences[regdst_num] = true;
    }

    return addr;
}

byte* CPU::vlen(byte* addr) {
    /*  Run vlen instruction.
     */
    bool regvec_ref, regval_ref;
    int regvec_num, regval_num;

    regvec_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regvec_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regval_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regval_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (regvec_ref ? " @" : " ") << regvec_num;
        cout << (regval_ref ? " @" : " ") << regval_num;
    }

    if (regvec_ref) {
        if (debug) { cout << "resolving reference to 1-operand register" << endl; }
        regvec_num = static_cast<Integer*>(uregisters[regvec_num])->value();
    }
    if (regval_ref) {
        if (debug) { cout << "resolving reference to 2-operand register" << endl; }
        regval_num = static_cast<Integer*>(uregisters[regval_num])->value();
    }

    place(regval_num, new Integer(static_cast<Vector*>(fetch(regvec_num))->len()));

    return addr;
}
