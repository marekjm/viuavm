#include <string>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/float.h"
#include "../../types/byte.h"
#include "../../types/string.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::itof(byte* addr) {
    /*  Run itof instruction.
     */
    bool casted_object_ref, destination_register_ref;
    int casted_object_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    casted_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    casted_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (casted_object_ref) {
        casted_object_index = static_cast<Integer*>(fetch(casted_object_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Float(static_cast<Integer*>(fetch(casted_object_index))->value()));

    return addr;
}

byte* CPU::ftoi(byte* addr) {
    /*  Run ftoi instruction.
     */
    bool casted_object_ref, destination_register_ref;
    int casted_object_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    casted_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    casted_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (casted_object_ref) {
        casted_object_index = static_cast<Integer*>(fetch(casted_object_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Integer(static_cast<Float*>(fetch(casted_object_index))->value()));

    return addr;
}

byte* CPU::stoi(byte* addr) {
    /*  Run stoi instruction.
     */
    bool casted_object_ref, destination_register_ref;
    int casted_object_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    casted_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    casted_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (casted_object_ref) {
        casted_object_index = static_cast<Integer*>(fetch(casted_object_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Integer(std::stoi(static_cast<String*>(fetch(casted_object_index))->value())));

    return addr;
}

byte* CPU::stof(byte* addr) {
    /*  Run stof instruction.
     */
    bool casted_object_ref, destination_register_ref;
    int casted_object_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    casted_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    casted_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (casted_object_ref) {
        casted_object_index = static_cast<Integer*>(fetch(casted_object_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Float(std::stod(static_cast<String*>(fetch(casted_object_index))->value())));

    return addr;
}
