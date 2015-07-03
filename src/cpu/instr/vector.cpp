#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/vector.h"
#include "../../support/pointer.h"
#include "../registerset.h"
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

    if (reg_ref) {
        reg_num = static_cast<Integer*>(fetch(reg_num))->value();
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

    if (regvec_ref) {
        regvec_num = static_cast<Integer*>(fetch(regvec_num))->value();
    }
    if (regval_ref) {
        regval_num = static_cast<Integer*>(fetch(regval_num))->value();
    }
    if (regpos_ref) {
        regpos_num = static_cast<Integer*>(fetch(regpos_num))->value();
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

    if (regvec_ref) {
        regvec_num = static_cast<Integer*>(fetch(regvec_num))->value();
    }
    if (regval_ref) {
        regval_num = static_cast<Integer*>(fetch(regval_num))->value();
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

    if (regvec_ref) {
        regvec_num = static_cast<Integer*>(fetch(regvec_num))->value();
    }
    if (regdst_ref) {
        regdst_num = static_cast<Integer*>(fetch(regdst_num))->value();
    }
    if (regpos_ref) {
        regpos_num = static_cast<Integer*>(fetch(regpos_num))->value();
    }

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Type* ptr = static_cast<Vector*>(fetch(regvec_num))->pop(regpos_num);
    if (regdst_num) { place(regdst_num, ptr); }

    return addr;
}

byte* CPU::vat(byte* addr) {
    /*  Run vat instruction.
     *
     *  Vector always returns a copy of the object in a register.
     *  FIXME: make it possible to pop references.
     */
    bool vector_operand_ref, destination_register_ref, position_operand_ref;
    int vector_operand_index, destination_register_index, position_operand_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    vector_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    vector_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    position_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    position_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (vector_operand_ref) {
        vector_operand_index = static_cast<Integer*>(fetch(vector_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }
    if (position_operand_ref) {
        position_operand_index = static_cast<Integer*>(fetch(position_operand_index))->value();
    }

    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    Type* ptr = static_cast<Vector*>(fetch(vector_operand_index))->at(position_operand_index);
    if (destination_register_index) {
        place(destination_register_index, ptr);
        uregset->flag(destination_register_index, REFERENCE);
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

    if (regvec_ref) {
        regvec_num = static_cast<Integer*>(fetch(regvec_num))->value();
    }
    if (regval_ref) {
        regval_num = static_cast<Integer*>(fetch(regval_num))->value();
    }

    place(regval_num, new Integer(static_cast<Vector*>(fetch(regvec_num))->len()));

    return addr;
}
