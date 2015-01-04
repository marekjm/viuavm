#include <iostream>
#include <vector>
#include "../../bytecode.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


char* CPU::istore(char* addr) {
    /*  Run istore instruction.
     */
    int reg, num;
    bool reg_ref = false, num_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    reg = *((int*)addr);
    pointer::inc<int, char>(addr);

    num_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ISTORE";
        cout << (reg_ref ? " @" : " ") << reg;
        cout << (num_ref ? " @" : " ") << num;
        cout << endl;
    }

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }
    if (num_ref) {
        num = static_cast<Integer*>(fetch(num))->value();
    }

    place(reg, new Integer(num));

    return addr;
}

char* CPU::iadd(char* addr) {
    /*  Run iadd instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IADD";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num + regb_num));

    return addr;
}

char* CPU::isub(char* addr) {
    /*  Run isub instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ISUB";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num - regb_num));

    return addr;
}

char* CPU::imul(char* addr) {
    /*  Run imul instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IMUL";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num * regb_num));

    return addr;
}

char* CPU::idiv(char* addr) {
    /*  Run idiv instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ISUB";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num / regb_num));

    return addr;
}

char* CPU::ilt(char* addr) {
    /*  Run ilt instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ILT";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num < regb_num));

    return addr;
}

char* CPU::ilte(char* addr) {
    /*  Run ilte instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ILTE";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num <= regb_num));

    return addr;
}

char* CPU::igt(char* addr) {
    /*  Run igt instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IGT";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num > regb_num));

    return addr;
}

char* CPU::igte(char* addr) {
    /*  Run igte instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IGTE";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num >= regb_num));

    return addr;
}

char* CPU::ieq(char* addr) {
    /*  Run ieq instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IEQ";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
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

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num == regb_num));

    return addr;
}

char* CPU::iinc(char* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regno = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IINC" << (ref ? " @" : " ") << regno;
    }

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    if (debug) {
        if (ref) { cout << " -> " << regno; }
        cout << endl;
    }

    ++(static_cast<Integer*>(fetch(regno))->value());

    return addr;
}

char* CPU::idec(char* addr) {
    /*  Run idec instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regno = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IDEC" << (ref ? " @" : " ") << regno;
    }

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    if (debug) {
        if (ref) { cout << " -> " << regno; }
        cout << endl;
    }

    --(static_cast<Integer*>(fetch(regno))->value());

    return addr;
}
