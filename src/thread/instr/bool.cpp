#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/boolean.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::lognot(byte* addr) {
    auto target = viua::operand::extract(addr);

    place(viua::operand::getRegisterIndexOrException(target.get(), this), new Boolean(not target->resolve(this)->boolean()));

    return addr;
}

byte* Thread::logand(byte* addr) {
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    place(viua::operand::getRegisterIndexOrException(target.get(), this), new Boolean(first->resolve(this)->boolean() and second->resolve(this)->boolean()));

    return addr;
}

byte* Thread::logor(byte* addr) {
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    place(viua::operand::getRegisterIndexOrException(target.get(), this), new Boolean(first->resolve(this)->boolean() or second->resolve(this)->boolean()));

    return addr;
}
