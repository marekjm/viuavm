#include <cstdint>
#include <iostream>
#include <viua/types/boolean.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::opecho(byte* addr) {
    cout << viua::operand::extract(addr)->resolve(this)->str();
    return addr;
}

byte* Process::opprint(byte* addr) {
    addr = opecho(addr);
    cout << '\n';
    return addr;
}


byte* Process::opjump(byte* addr) {
    uint64_t* offset = reinterpret_cast<uint64_t*>(addr);
    byte* target = (jump_base+(*offset));
    if (target == addr) {
        throw new Exception("aborting: JUMP instruction pointing to itself");
    }
    return target;
}

byte* Process::opbranch(byte* addr) {
    Type* condition = viua::operand::extract(addr)->resolve(this);

    uint64_t addr_true, addr_false;
    viua::cpu::util::extractOperand<decltype(addr_true)>(addr, addr_true);
    viua::cpu::util::extractOperand<decltype(addr_false)>(addr, addr_false);

    return (jump_base + (condition->boolean() ? addr_true : addr_false));
}
