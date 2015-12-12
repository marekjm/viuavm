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
    place(viua::operand::getRegisterIndexOrException(viua::operand::extract(addr).get(), this), new Integer(0));
    return addr;
}

byte* Thread::istore(byte* addr) {
    /*  Run istore instruction.
     */
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
    /*  Run iadd instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() + static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::isub(byte* addr) {
    /*  Run isub instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() - static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::imul(byte* addr) {
    /*  Run imul instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() * static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::idiv(byte* addr) {
    /*  Run idiv instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Integer(static_cast<Integer*>(first->resolve(this))->as_integer() / static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::ilt(byte* addr) {
    /*  Run ilt instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() < static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::ilte(byte* addr) {
    /*  Run ilte instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() <= static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::igt(byte* addr) {
    /*  Run igt instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() > static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::igte(byte* addr) {
    /*  Run igte instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() >= static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::ieq(byte* addr) {
    /*  Run ieq instruction.
     */
    auto target = viua::operand::extract(addr);

    unsigned target_register_index = 0;
    if (viua::operand::RegisterIndex* ri = dynamic_cast<viua::operand::RegisterIndex*>(target.get())) {
        target_register_index = ri->get(this);
    } else {
        throw new Exception("invalid operand type");
    }

    auto first = viua::operand::extract(addr);
    auto second = viua::operand::extract(addr);
    place(target_register_index, new Boolean(static_cast<Integer*>(first->resolve(this))->as_integer() == static_cast<Integer*>(second->resolve(this))->as_integer()));

    return addr;
}

byte* Thread::iinc(byte* addr) {
    /*  Run iinc instruction.
     */
    static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->increment();
    return addr;
}

byte* Thread::idec(byte* addr) {
    /*  Run idec instruction.
     */
    static_cast<Integer*>(viua::operand::extract(addr)->resolve(this))->decrement();
    return addr;
}
