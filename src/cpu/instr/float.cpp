#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/boolean.h"
#include "../../types/integer.h"
#include "../../types/float.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::fstore(byte* addr) {
    /*  Run istore instruction.
     */
    int destination_register_index;
    float value;
    bool destination_register_ref = false;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    value = *((float*)addr);
    pointer::inc<float, byte>(addr);

    if (destination_register_ref) {
        destination_register_index = static_cast<Integer*>(fetch(destination_register_index))->value();
    }

    place(destination_register_index, new Float(value));

    return addr;
}

byte* CPU::fadd(byte* addr) {
    /*  Run fadd instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Float(a + b));

    return addr;
}

byte* CPU::fsub(byte* addr) {
    /*  Run fsub instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Float(a - b));

    return addr;
}

byte* CPU::fmul(byte* addr) {
    /*  Run fmul instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Float(a * b));

    return addr;
}

byte* CPU::fdiv(byte* addr) {
    /*  Run fdiv instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Float(a / b));

    return addr;
}

byte* CPU::flt(byte* addr) {
    /*  Run flt instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Boolean(a < b));

    return addr;
}

byte* CPU::flte(byte* addr) {
    /*  Run flte instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Boolean(a <= b));

    return addr;
}

byte* CPU::fgt(byte* addr) {
    /*  Run fgt instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Boolean(a > b));

    return addr;
}

byte* CPU::fgte(byte* addr) {
    /*  Run fgte instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Boolean(a >= b));

    return addr;
}

byte* CPU::feq(byte* addr) {
    /*  Run feq instruction.
     */
    bool first_operand_ref, second_operand_ref, destination_register_ref;
    int first_operand_index, second_operand_index, destination_register_index;

    destination_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    destination_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    first_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    first_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    second_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    second_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (first_operand_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }
    if (second_operand_ref) {
        second_operand_index = static_cast<Integer*>(fetch(second_operand_index))->value();
    }
    if (destination_register_ref) {
        first_operand_index = static_cast<Integer*>(fetch(first_operand_index))->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(first_operand_index))->value();
    b = static_cast<Float*>(fetch(second_operand_index))->value();

    place(destination_register_index, new Boolean(a == b));

    return addr;
}
