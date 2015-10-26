#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/boolean.h>
#include <viua/types/byte.h>
#include <viua/types/casts/integer.h>
#include <viua/support/pointer.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::izero(byte* addr) {
    /*  Run istore instruction.
     */
    int destination_register;
    bool destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (destination_register_ref) {
        destination_register = static_cast<Integer*>(fetch(destination_register))->value();
    }

    place(destination_register, new Integer(0));

    return addr;
}

byte* Thread::istore(byte* addr) {
    /*  Run istore instruction.
     */
    int destination_register, operand;
    bool destination_register_ref = false, operand_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register = *((int*)addr);
    pointer::inc<int, byte>(addr);

    operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    operand = *((int*)addr);
    pointer::inc<int, byte>(addr);

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
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Integer(first_operand_num + second_operand_num));

    return addr;
}

byte* Thread::isub(byte* addr) {
    /*  Run isub instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Integer(first_operand_num - second_operand_num));

    return addr;
}

byte* Thread::imul(byte* addr) {
    /*  Run imul instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Integer(first_operand_num * second_operand_num));

    return addr;
}

byte* Thread::idiv(byte* addr) {
    /*  Run idiv instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Integer(first_operand_num / second_operand_num));

    return addr;
}

byte* Thread::ilt(byte* addr) {
    /*  Run ilt instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Boolean(first_operand_num < second_operand_num));

    return addr;
}

byte* Thread::ilte(byte* addr) {
    /*  Run ilte instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Boolean(first_operand_num <= second_operand_num));

    return addr;
}

byte* Thread::igt(byte* addr) {
    /*  Run igt instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Boolean(first_operand_num > second_operand_num));

    return addr;
}

byte* Thread::igte(byte* addr) {
    /*  Run igte instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Boolean(first_operand_num >= second_operand_num));

    return addr;
}

byte* Thread::ieq(byte* addr) {
    /*  Run ieq instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_num, second_operand_num, destination_register_num;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }
    if (second_operand_ref) {
        second_operand_num = static_cast<Integer*>(fetch(second_operand_num))->value();
    }
    if (destination_register_ref) {
        first_operand_num = static_cast<Integer*>(fetch(first_operand_num))->value();
    }

    first_operand_num = static_cast<IntegerCast*>(fetch(first_operand_num))->as_integer();
    second_operand_num = static_cast<IntegerCast*>(fetch(second_operand_num))->as_integer();

    place(destination_register_num, new Boolean(first_operand_num == second_operand_num));

    return addr;
}

byte* Thread::iinc(byte* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int target_register;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    target_register = *((int*)addr);
    pointer::inc<int, byte>(addr);

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

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    target_register = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        target_register = static_cast<Integer*>(fetch(target_register))->value();
    }

    static_cast<IntegerCast*>(fetch(target_register))->decrement();

    return addr;
}
