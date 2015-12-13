#include <memory>
#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/casts/integer.h>
#include <viua/exceptions.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::izero(byte* addr) {
    place(viua::operand::getRegisterIndexOrException(viua::operand::extract(addr).get(), this), new Integer(0));
    return addr;
}

byte* Thread::istore(byte* addr) {
    unsigned destination_register = viua::operand::getRegisterIndexOrException(viua::operand::extract(addr).get(), this);

    auto source = viua::operand::extract(addr);
    int integer = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(source.get())) {
        integer = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(destination_register, new Integer(integer));

    return addr;
}

byte* Thread::iadd(byte* addr) {
    return performBinaryOperation<std::plus<int>, Integer>(addr);
}

byte* Thread::isub(byte* addr) {
    return performBinaryOperation<std::minus<int>, Integer>(addr);
}

byte* Thread::imul(byte* addr) {
    return performBinaryOperation<std::multiplies<int>, Integer>(addr);
}

byte* Thread::idiv(byte* addr) {
    return performBinaryOperation<std::divides<int>, Integer>(addr);
}

byte* Thread::ilt(byte* addr) {
    return performBinaryOperation<std::less<int>, Boolean>(addr);
}

byte* Thread::ilte(byte* addr) {
    return performBinaryOperation<std::less_equal<int>, Boolean>(addr);
}

byte* Thread::igt(byte* addr) {
    return performBinaryOperation<std::greater<int>, Boolean>(addr);
}

byte* Thread::igte(byte* addr) {
    return performBinaryOperation<std::greater_equal<int>, Boolean>(addr);
}

byte* Thread::ieq(byte* addr) {
    return performBinaryOperation<std::equal_to<int>, Boolean>(addr);
}

byte* Thread::iinc(byte* addr) {
    static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->increment();
    return addr;
}

byte* Thread::idec(byte* addr) {
    static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->decrement();
    return addr;
}
