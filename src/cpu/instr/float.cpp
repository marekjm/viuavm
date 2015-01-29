#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/boolean.h"
#include "../../types/integer.h"
#include "../../types/float.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::fstore(byte* addr) {
    /*  Run istore instruction.
     */
    int reg;
    float value;
    bool ref = false;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    value = *((float*)addr);
    pointer::inc<float, byte>(addr);

    if (debug) {
        cout << (ref ? " @" : " ") << reg;
        cout << ' ' << value;
    }

    if (ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new Float(value));

    return addr;
}

byte* CPU::fadd(byte* addr) {
    /*  Run fadd instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Float(a + b));

    return addr;
}

byte* CPU::fsub(byte* addr) {
    /*  Run fsub instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Float(a - b));

    return addr;
}

byte* CPU::fmul(byte* addr) {
    /*  Run fmul instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Float(a * b));

    return addr;
}

byte* CPU::fdiv(byte* addr) {
    /*  Run fdiv instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Float(a / b));

    return addr;
}

byte* CPU::flt(byte* addr) {
    /*  Run flt instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(a < b));

    return addr;
}

byte* CPU::flte(byte* addr) {
    /*  Run flte instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(a <= b));

    return addr;
}

byte* CPU::fgt(byte* addr) {
    /*  Run fgt instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(a > b));

    return addr;
}

byte* CPU::fgte(byte* addr) {
    /*  Run fgte instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(a >= b));

    return addr;
}

byte* CPU::feq(byte* addr) {
    /*  Run feq instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    float a, b;
    a = static_cast<Float*>(fetch(rega_num))->value();
    b = static_cast<Float*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(a == b));

    return addr;
}
