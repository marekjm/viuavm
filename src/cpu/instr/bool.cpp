#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
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
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }

    place(destination_register_index, new Boolean(fetch(first_operand_index)->boolean() and fetch(second_operand_index)->boolean()));

    return addr;
}

byte* CPU::logor(byte* addr) {
    /*  Run ieq instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }

    place(destination_register_index, new Boolean(fetch(first_operand_index)->boolean() or fetch(second_operand_index)->boolean()));

    return addr;
}
