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
    bool target_register_ref;
    int target_register_index;

    target_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    target_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (target_register_ref) {
        target_register_index = static_cast<Integer*>(fetch(target_register_index))->value();
    }

    place(target_register_index, new Vector());

    return addr;
}

byte* CPU::vinsert(byte* addr) {
    /*  Run vinsert instruction.
     *
     *  Vector always inserts a copy of the object in a register.
     *  FIXME: make it possible to insert references.
     */
    bool vector_operand_ref, object_operand_ref, position_operand_ref;
    int vector_operand_index, object_operand_index, position_operand_index;

    vector_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    vector_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    position_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    position_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (vector_operand_ref) {
        vector_operand_index = static_cast<Integer*>(fetch(vector_operand_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }
    if (position_operand_ref) {
        position_operand_index = static_cast<Integer*>(fetch(position_operand_index))->value();
    }

    static_cast<Vector*>(fetch(vector_operand_index))->insert(position_operand_index, fetch(object_operand_index)->copy());

    return addr;
}

byte* CPU::vpush(byte* addr) {
    /*  Run vpush instruction.
     *
     *  Vector always pushes a copy of the object in a register.
     *  FIXME: make it possible to push references.
     */
    bool vector_operand_ref, object_operand_ref;
    int vector_operand_index, object_operand_index;

    vector_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    vector_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (vector_operand_ref) {
        vector_operand_index = static_cast<Integer*>(fetch(vector_operand_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    static_cast<Vector*>(fetch(vector_operand_index))->push(fetch(object_operand_index)->copy());

    return addr;
}

byte* CPU::vpop(byte* addr) {
    /*  Run vpop instruction.
     *
     *  Vector always pops a copy of the object in a register.
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
    Type* ptr = static_cast<Vector*>(fetch(vector_operand_index))->pop(position_operand_index);
    if (destination_register_index) { place(destination_register_index, ptr); }

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
    place(destination_register_index, ptr);
    uregset->flag(destination_register_index, REFERENCE);

    return addr;
}

byte* CPU::vlen(byte* addr) {
    /*  Run vlen instruction.
     */
    bool vector_operand_ref, destination_register_ref;
    int vector_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    vector_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    vector_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (vector_operand_ref) {
        vector_operand_index = static_cast<Integer*>(fetch(vector_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Integer(static_cast<Vector*>(fetch(vector_operand_index))->len()));

    return addr;
}
