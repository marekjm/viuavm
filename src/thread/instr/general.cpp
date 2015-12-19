#include <cstdint>
#include <iostream>
#include <viua/types/boolean.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::echo(byte* addr) {
    cout << viua::operand::extract(addr)->resolve(this)->str();
    return addr;
}

byte* Thread::print(byte* addr) {
    addr = echo(addr);
    cout << '\n';
    return addr;
}


byte* Thread::jump(byte* addr) {
    /*  Run jump instruction.
     */
    uint64_t* offset = reinterpret_cast<uint64_t*>(addr);
    byte* target = (jump_base+(*offset));
    if (target == addr) {
        throw new Exception("aborting: JUMP instruction pointing to itself");
    }
    return target;
}

byte* Thread::branch(byte* addr) {
    /*  Run branch instruction.
     */
    bool condition_object_ref;
    int condition_object_index;

    viua::cpu::util::extractIntegerOperand(addr, condition_object_ref, condition_object_index);

    uint64_t addr_true, addr_false;
    viua::cpu::util::extractOperand<decltype(addr_true)>(addr, addr_true);
    viua::cpu::util::extractOperand<decltype(addr_false)>(addr, addr_false);

    if (condition_object_ref) {
        condition_object_index = static_cast<Integer*>(fetch(condition_object_index))->value();
    }

    bool result = fetch(condition_object_index)->boolean();

    addr = jump_base + (result ? addr_true : addr_false);

    return addr;
}
