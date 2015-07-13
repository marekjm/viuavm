#include <iostream>
#include <viua/types/boolean.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* CPU::echo(byte* addr) {
    /*  Run echo instruction.
     */
    bool ref = false;
    int operand_index;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        operand_index = static_cast<Integer*>(fetch(operand_index))->value();
    }

    cout << fetch(operand_index)->str();

    return addr;
}

byte* CPU::print(byte* addr) {
    /*  Run print instruction.
     */
    addr = echo(addr);
    cout << '\n';
    return addr;
}


byte* CPU::jump(byte* addr) {
    /*  Run jump instruction.
     */
    byte* target = jump_base+(*(int*)addr);
    if (target == addr) {
        throw new Exception("aborting: JUMP instruction pointing to itself");
    }
    return target;
}

byte* CPU::branch(byte* addr) {
    /*  Run branch instruction.
     */
    bool condition_object_ref;
    int condition_object_index;

    condition_object_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    condition_object_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, byte>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (condition_object_ref) {
        condition_object_index = static_cast<Integer*>(fetch(condition_object_index))->value();
    }

    bool result = fetch(condition_object_index)->boolean();

    addr = jump_base + (result ? addr_true : addr_false);

    return addr;
}
