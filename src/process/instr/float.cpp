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


byte* Process::opfstore(byte* addr) {
    /*  Run fstore instruction.
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

// ObjectPlacer shall be used only as a type of Process::place() function when passing it
// to perform<>() implementation template.
using ObjectPlacer = void(Process::*)(unsigned,Type*);

template<class Operator, class ResultType> byte* perform(byte* addr, Process* t, ObjectPlacer placer) {
    /** Heavily abstracted binary opcode implementation for Float-related instructions.
     *
     *  First parameter - byte* addr - is the instruction pointer from which operand extraction should begin.
     *
     *  Second parameter - Process* t - is a pointer to current VM process (passed as `this`).
     *
     *  Third parameter - ObjectPlacer - is a member-function pointer to Process::place.
     *  Since it is private, we have to cheat the compiler by extracting its pointer while in
     *  Process class's scope and passing it here.
     *  Voila - we can place objects in process's current register set.
     */
    unsigned target_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), t);

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    (t->*placer)(target_register_index, new ResultType(Operator()(static_cast<Float*>(first->resolve(t))->value(), static_cast<Float*>(second->resolve(t))->value())));

    return addr;
}

byte* Process::opfadd(byte* addr) {
    return perform<std::plus<float>, Float>(addr, this, &Process::place);
}

byte* Process::opfsub(byte* addr) {
    return perform<std::minus<float>, Float>(addr, this, &Process::place);
}

byte* Process::opfmul(byte* addr) {
    return perform<std::multiplies<float>, Float>(addr, this, &Process::place);
}

byte* Process::opfdiv(byte* addr) {
    return perform<std::divides<float>, Float>(addr, this, &Process::place);
}

byte* Process::opflt(byte* addr) {
    return perform<std::less<float>, Boolean>(addr, this, &Process::place);
}

byte* Process::opflte(byte* addr) {
    return perform<std::less_equal<float>, Boolean>(addr, this, &Process::place);
}

byte* Process::opfgt(byte* addr) {
    return perform<std::greater<float>, Boolean>(addr, this, &Process::place);
}

byte* Process::opfgte(byte* addr) {
    return perform<std::greater_equal<float>, Boolean>(addr, this, &Process::place);
}

byte* Process::opfeq(byte* addr) {
    return perform<std::equal_to<float>, Boolean>(addr, this, &Process::place);
}
