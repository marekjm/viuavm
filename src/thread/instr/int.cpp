#include <memory>
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
    /*  Run istore instruction.
     */
    auto operand = viua::operand::extract(addr);

    if (viua::operand::RegisterIndex* ri =  dynamic_cast<viua::operand::RegisterIndex*>(operand.get())) {
        place(ri->get(this), new Integer(0));
    }

    return addr;
}

byte* Thread::istore(byte* addr) {
    /*  Run istore instruction.
     */
    auto target = viua::operand::extract(addr);
    auto source = viua::operand::extract(addr);

    int destination_register = -1;

    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        destination_register = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    int integer = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(source.get())) {
        integer = ri->get(this);
    } else if (viua::operand::RegisterReference* rr = dynamic_cast<viua::operand::RegisterReference*>(source.get())) {
        integer = static_cast<Integer*>(fetch(rr->get(this)))->value();
    }

    place(destination_register, new Integer(integer));

    return addr;
}

byte* Thread::iadd(byte* addr) {
    /*  Run iadd instruction.
     */
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() + static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::isub(byte* addr) {
    /*  Run isub instruction.
     */
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() - static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::imul(byte* addr) {
    /*  Run imul instruction.
     */
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() * static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::idiv(byte* addr) {
    /*  Run idiv instruction.
     */
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() / static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::ilt(byte* addr) {
    /*  Run ilt instruction.
     */
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() < static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::ilte(byte* addr) {
    /*  Run ilte instruction.
     */
    auto target = viua::operand::extract(addr);
    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() <= static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::igt(byte* addr) {
    /*  Run igt instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, first_operand_ref, first_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, second_operand_ref, second_operand_index);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    first_operand_index = static_cast<IntegerCast*>(fetch(first_operand_index))->as_integer();
    second_operand_index = static_cast<IntegerCast*>(fetch(second_operand_index))->as_integer();

    place(destination_register_index, new Boolean(first_operand_index > second_operand_index));

    return addr;
}

byte* Thread::igte(byte* addr) {
    /*  Run igte instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, first_operand_ref, first_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, second_operand_ref, second_operand_index);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    first_operand_index = static_cast<IntegerCast*>(fetch(first_operand_index))->as_integer();
    second_operand_index = static_cast<IntegerCast*>(fetch(second_operand_index))->as_integer();

    place(destination_register_index, new Boolean(first_operand_index >= second_operand_index));

    return addr;
}

byte* Thread::ieq(byte* addr) {
    /*  Run ieq instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);
    viua::cpu::util::extractIntegerOperand(addr, first_operand_ref, first_operand_index);
    viua::cpu::util::extractIntegerOperand(addr, second_operand_ref, second_operand_index);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    first_operand_index = static_cast<IntegerCast*>(fetch(first_operand_index))->as_integer();
    second_operand_index = static_cast<IntegerCast*>(fetch(second_operand_index))->as_integer();

    place(destination_register_index, new Boolean(first_operand_index == second_operand_index));

    return addr;
}

byte* Thread::iinc(byte* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int target_register;

    viua::cpu::util::extractIntegerOperand(addr, ref, target_register);

    if (ref) {
        target_register = static_cast<Integer*>(fetch(target_register))->value();
    }

    static_cast<IntegerCast*>(fetch(target_register))->increment();

    return addr;
}

byte* Thread::idec(byte* addr) {
    /*  Run idec instruction.
     */
    bool ref = false;
    int target_register;

    viua::cpu::util::extractIntegerOperand(addr, ref, target_register);

    if (ref) {
        target_register = static_cast<Integer*>(fetch(target_register))->value();
    }

    static_cast<IntegerCast*>(fetch(target_register))->decrement();

    return addr;
}
