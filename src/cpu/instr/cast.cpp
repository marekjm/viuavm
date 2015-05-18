#include <string>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/float.h"
#include "../../types/byte.h"
#include "../../types/string.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::itof(byte* addr) {
    /*  Run itof instruction.
     */
    bool rega_ref, regb_ref;
    int rega_num, regb_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }

    place(regb_num, new Float(static_cast<Integer*>(fetch(rega_num))->value()));

    return addr;
}

byte* CPU::ftoi(byte* addr) {
    /*  Run ftoi instruction.
     */
    bool rega_ref, regb_ref;
    int rega_num, regb_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }

    place(regb_num, new Integer(static_cast<Float*>(fetch(rega_num))->value()));

    return addr;
}

byte* CPU::stoi(byte* addr) {
    /*  Run stoi instruction.
     */
    bool rega_ref, regb_ref;
    int rega_num, regb_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }

    place(regb_num, new Integer(std::stoi(static_cast<String*>(fetch(rega_num))->value())));

    return addr;
}

byte* CPU::stof(byte* addr) {
    /*  Run stof instruction.
     */
    bool rega_ref, regb_ref;
    int rega_num, regb_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (rega_ref) {
        rega_num = static_cast<Integer*>(fetch(rega_num))->value();
    }
    if (regb_ref) {
        regb_num = static_cast<Integer*>(fetch(regb_num))->value();
    }

    place(regb_num, new Float(std::stod(static_cast<String*>(fetch(rega_num))->value())));

    return addr;
}
