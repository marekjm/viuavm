#include <iostream>
#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/cpu/opex.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::fstore(byte* addr) {
    /*  Run istore instruction.
     */
    int destination_register_index;
    float value;
    bool destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractFloatingPointOperand(addr, value);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Float(value));

    return addr;
}

// ObjectPlacer shall be used only as a type of Thread::place() function when passing it
// to perform<>() implementation template.
using ObjectPlacer = void(Thread::*)(unsigned,Type*);

template<class Operator, class ResultType> byte* perform(byte* addr, Thread* t, ObjectPlacer placer) {
    /** Heavily abstracted binary opcode implementation for Integer-related instructions.
     *
     *  First parameter - byte* addr - is the instruction pointer from which operand extraction should begin.
     *
     *  Second parameter - Thread* t - is a pointer to current VM thread of execution (passed as `this`).
     *
     *  Third parameter - ObjectPlacer - is a member-function pointer to Thread::place.
     *  Since it is private, we have to cheat the compiler by extracting its pointer while in
     *  Thread class's scope and passing it here.
     *  Voila - we can place objects in thread's current register set.
     */
    unsigned target_register_index = viua::operand::getRegisterIndexOrException(viua::operand::extract(addr).get(), t);

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    (t->*placer)(target_register_index, new ResultType(Operator()(static_cast<Float*>(first->resolve(t))->value(), static_cast<Float*>(second->resolve(t))->value())));

    return addr;
}

byte* Thread::fadd(byte* addr) {
    return perform<std::plus<float>, Float>(addr, this, &Thread::place);
}

byte* Thread::fsub(byte* addr) {
    return perform<std::minus<float>, Float>(addr, this, &Thread::place);
}

byte* Thread::fmul(byte* addr) {
    return perform<std::multiplies<float>, Float>(addr, this, &Thread::place);
}

byte* Thread::fdiv(byte* addr) {
    return perform<std::divides<float>, Float>(addr, this, &Thread::place);
}

byte* Thread::flt(byte* addr) {
    return perform<std::less<float>, Boolean>(addr, this, &Thread::place);
}

byte* Thread::flte(byte* addr) {
    return perform<std::less_equal<float>, Boolean>(addr, this, &Thread::place);
}

byte* Thread::fgt(byte* addr) {
    return perform<std::greater<float>, Boolean>(addr, this, &Thread::place);
}

byte* Thread::fgte(byte* addr) {
    return perform<std::greater_equal<float>, Boolean>(addr, this, &Thread::place);
}

byte* Thread::feq(byte* addr) {
    return perform<std::equal_to<float>, Boolean>(addr, this, &Thread::place);
}
