#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::izero(byte* addr) {
    /*  Run istore instruction.
     */
    int reg;
    bool reg_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new Integer(0));

    return addr;
}

byte* CPU::istore(byte* addr) {
    /*  Run istore instruction.
     */
    int reg, num;
    bool reg_ref = false, num_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    num_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }
    if (num_ref) {
        num = static_cast<Integer*>(fetch(num))->value();
    }

    place(reg, new Integer(num));

    return addr;
}

byte* CPU::iadd(byte* addr) {
    /*  Run iadd instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num + regb_num));

    return addr;
}

byte* CPU::isub(byte* addr) {
    /*  Run isub instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num - regb_num));

    return addr;
}

byte* CPU::imul(byte* addr) {
    /*  Run imul instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num * regb_num));

    return addr;
}

byte* CPU::idiv(byte* addr) {
    /*  Run idiv instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Integer(rega_num / regb_num));

    return addr;
}

byte* CPU::ilt(byte* addr) {
    /*  Run ilt instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num < regb_num));

    return addr;
}

byte* CPU::ilte(byte* addr) {
    /*  Run ilte instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num <= regb_num));

    return addr;
}

byte* CPU::igt(byte* addr) {
    /*  Run igt instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num > regb_num));

    return addr;
}

byte* CPU::igte(byte* addr) {
    /*  Run igte instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num >= regb_num));

    return addr;
}

byte* CPU::ieq(byte* addr) {
    /*  Run ieq instruction.
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

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }
    if (regr_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }

    rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    regb_num = static_cast<Integer*>(fetch(regb_num))->value();

    place(regr_num, new Boolean(rega_num == regb_num));

    return addr;
}

byte* CPU::iinc(byte* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regno = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }

    ++(static_cast<Integer*>(fetch(regno))->value());

    return addr;
}

byte* CPU::idec(byte* addr) {
    /*  Run idec instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);

    regno = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        regno = static_cast<Integer*>(fetch(regno))->value();
    }
    --(static_cast<Integer*>(fetch(regno))->value());

    return addr;
}
