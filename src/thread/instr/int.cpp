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


byte* Process::opizero(byte* addr) {
    place(viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this), new Integer(0));
    return addr;
}

byte* Process::opistore(byte* addr) {
    unsigned destination_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

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

// ObjectPlacer shall be used only as a type of Process::place() function when passing it
// to perform<>() implementation template.
using ObjectPlacer = void(Process::*)(unsigned,Type*);

template<class Operator, class ResultType> byte* perform(byte* addr, Process* t, ObjectPlacer placer) {
    /** Heavily abstracted binary opcode implementation for Integer-related instructions.
     *
     *  First parameter - byte* addr - is the instruction pointer from which operand extraction should begin.
     *
     *  Second parameter - Process* t - is a pointer to current VM thread of execution (passed as `this`).
     *
     *  Third parameter - ObjectPlacer - is a member-function pointer to Process::place.
     *  Since it is private, we have to cheat the compiler by extracting its pointer while in
     *  Process class's scope and passing it here.
     *  Voila - we can place objects in thread's current register set.
     */
    unsigned target_register_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), t);

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    (t->*placer)(target_register_index, new ResultType(Operator()(static_cast<Integer*>(first->resolve(t))->as_integer(), static_cast<Integer*>(second->resolve(t))->as_integer())));

    return addr;
}

byte* Process::opiadd(byte* addr) {
    return perform<std::plus<int>, Integer>(addr, this, &Process::place);
}

byte* Process::opisub(byte* addr) {
    return perform<std::minus<int>, Integer>(addr, this, &Process::place);
}

byte* Process::opimul(byte* addr) {
    return perform<std::multiplies<int>, Integer>(addr, this, &Process::place);
}

byte* Process::opidiv(byte* addr) {
    return perform<std::divides<int>, Integer>(addr, this, &Process::place);
}

byte* Process::opilt(byte* addr) {
    return perform<std::less<int>, Boolean>(addr, this, &Process::place);
}

byte* Process::opilte(byte* addr) {
    return perform<std::less_equal<int>, Boolean>(addr, this, &Process::place);
}

byte* Process::opigt(byte* addr) {
    return perform<std::greater<int>, Boolean>(addr, this, &Process::place);
}

byte* Process::opigte(byte* addr) {
    return perform<std::greater_equal<int>, Boolean>(addr, this, &Process::place);
}

byte* Process::opieq(byte* addr) {
    return perform<std::equal_to<int>, Boolean>(addr, this, &Process::place);
}

byte* Process::opiinc(byte* addr) {
    static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->increment();
    return addr;
}

byte* Process::opidec(byte* addr) {
    static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->decrement();
    return addr;
}
