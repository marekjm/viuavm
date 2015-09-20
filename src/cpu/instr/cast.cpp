#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/types/exception.h>
#include <viua/support/pointer.h>
#include <viua/cpu/cpu.h>
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
    bool cast_object_ref, destination_register_ref;
    int cast_object_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    cast_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    cast_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (cast_object_ref) {
        cast_object_index = static_cast<Integer*>(fetch(cast_object_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    int result_integer = 0;
    string supplied_string = static_cast<String*>(fetch(cast_object_index))->value();
    try {
        result_integer = std::stoi(supplied_string);
    } catch (const std::out_of_range& e) {
        throw new Exception("out of range: " + supplied_string);
    }
    place(destination_register_index, new Integer(result_integer));

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
