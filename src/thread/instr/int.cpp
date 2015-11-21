#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/casts/integer.h>
#include <viua/cpu/opex.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::izero(byte* addr) {
    /*  Run istore instruction.
     */
    int destination_register_index;
    bool destination_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register_index);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Integer(0));

    return addr;
}

byte* Thread::istore(byte* addr) {
    /*  Run istore instruction.
     */
    int destination_register, operand;
    bool destination_register_ref = false, operand_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, destination_register_ref, destination_register);
    viua::cpu::util::extractIntegerOperand(addr, operand_ref, operand);

    if (destination_register_ref) {
        destination_register = static_cast<Integer*>(fetch(destination_register))->value();
    }
    if (operand_ref) {
        operand = static_cast<Integer*>(fetch(operand))->value();
    }

    place(destination_register, new Integer(operand));

    return addr;
}

byte* Thread::iadd(byte* addr) {
    /*  Run iadd instruction.
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

    place(destination_register_index, new Integer(first_operand_index + second_operand_index));

    return addr;
}

byte* Thread::isub(byte* addr) {
    /*  Run isub instruction.
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

    place(destination_register_index, new Integer(first_operand_index - second_operand_index));

    return addr;
}

byte* Thread::imul(byte* addr) {
    /*  Run imul instruction.
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

    place(destination_register_index, new Integer(first_operand_index * second_operand_index));

    return addr;
}

byte* Thread::idiv(byte* addr) {
    /*  Run idiv instruction.
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

    place(destination_register_index, new Integer(first_operand_index / second_operand_index));

    return addr;
}

byte* Thread::ilt(byte* addr) {
    /*  Run ilt instruction.
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

    place(destination_register_index, new Boolean(first_operand_index < second_operand_index));

    return addr;
}

byte* Thread::ilte(byte* addr) {
    /*  Run ilte instruction.
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

    place(destination_register_index, new Boolean(first_operand_index <= second_operand_index));

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
