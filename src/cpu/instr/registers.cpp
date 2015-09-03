#include <iostream>
#include <viua/types/boolean.h>
#include <viua/types/reference.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* CPU::move(byte* addr) {
    /** Run move instruction.
     *  Move an object from one register into another.
     */
    int object_operand_index, destination_register_index;
    bool object_operand_ref = false, destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    uregset->move(object_operand_index, destination_register_index);
    return addr;
}
byte* CPU::copy(byte* addr) {
    /** Run copy instruction.
     *  Copy an object from one register into another.
     */
    int object_operand_index, destination_register_index;
    bool object_operand_ref = false, destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, fetch(object_operand_index)->copy());

    return addr;
}
byte* CPU::ref(byte* addr) {
    /** Run ref instruction.
     *  Create object_operand_index reference (implementation detail: copy object_operand_index pointer) of an object in one register in
     *  another register.
     */
    int object_operand_index, destination_register_index;
    bool object_operand_ref = false, destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    Type* object = uregset->at(object_operand_index);
    Reference* rf = nullptr;
    if (dynamic_cast<Reference*>(object)) {
        //cout << "thou shalt not reference references!" << endl;
        rf = static_cast<Reference*>(object)->copy();
        uregset->set(destination_register_index, rf);
    } else {
        rf = new Reference(object);
        uregset->empty(object_operand_index);
        uregset->set(object_operand_index, rf);
        uregset->set(destination_register_index, rf->copy());
    }

    return addr;
}
byte* CPU::swap(byte* addr) {
    /** Run swap instruction.
     *  Swaps two objects in registers.
     */
    int first_operand_index, second_operand_index;
    bool first_operand_ref = false, second_operand_ref = false;

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }

    uregset->swap(first_operand_index, second_operand_index);
    return addr;
}
byte* CPU::free(byte* addr) {
    /** Run free instruction.
     */
    int target_register_index;
    bool target_register_ref = false;

    target_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    target_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (target_register_ref) {
        target_register_index = static_cast<Integer*>(fetch(target_register_index))->value();
    }

    uregset->free(target_register_index);

    return addr;
}
byte* CPU::empty(byte* addr) {
    /** Run empty instruction.
     */
    int target_register_index;
    bool target_register_ref = false;

    target_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    target_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (target_register_ref) {
        target_register_index = static_cast<Integer*>(fetch(target_register_index))->value();
    }

    uregset->empty(target_register_index);

    return addr;
}
byte* CPU::isnull(byte* addr) {
    /** Run isnull instruction.
     *
     * Example:
     *
     *      isnull checked_register_index, B
     *
     * the above means: "check if checked_register_index is null and store the information in B".
     */
    int checked_register_index, destination_register_index;
    bool checked_register_ref = false, destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    checked_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    checked_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (checked_register_ref) {
        checked_register_index = static_cast<Integer*>(fetch(checked_register_index))->value();
    }
    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Boolean(uregset->at(checked_register_index) == 0));

    return addr;
}

byte* CPU::ress(byte* addr) {
    /*  Run ress instruction.
     */
    int to_register_set = 0;
    to_register_set = *(int*)addr;
    pointer::inc<int, byte>(addr);

    switch (to_register_set) {
        case 0:
            uregset = regset;
            break;
        case 1:
            uregset = frames.back()->regset;
            break;
        case 2:
            ensureStaticRegisters(frames.back()->function_name);
            uregset = static_registers.at(frames.back()->function_name);
            break;
        case 3:
            // TODO: switching to temporary registers
        default:
            throw new Exception("illegal register set ID in ress instruction");
    }

    return addr;
}

byte* CPU::tmpri(byte* addr) {
    /** Run tmpri instruction.
     */
    int object_operand_index;
    bool object_operand_ref = false;

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    if (tmp != 0) {
        cout << "warning: CPU: storing in non-empty temporary register: memory has been leaked" << endl;
    }
    tmp = uregset->get(object_operand_index)->copy();

    return addr;
}
byte* CPU::tmpro(byte* addr) {
    /** Run tmpro instruction.
     */
    int destination_register_index;
    bool destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    if (uregset->at(destination_register_index) != 0) {
        if (errors) {
            cerr << "warning: CPU: droping from temporary into non-empty register: possible references loss and register corruption" << endl;
        }
        uregset->free(destination_register_index);
    }
    uregset->set(destination_register_index, tmp);
    tmp = 0;

    return addr;
}
