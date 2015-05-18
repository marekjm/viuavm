#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/boolean.h"
#include "../../types/byte.h"
#include "../../types/string.h"
#include "../../support/pointer.h"
#include "../../support/string.h"
#include "../cpu.h"
using namespace std;


byte* CPU::strstore(byte* addr) {
    /*  Run strstore instruction.
     */
    int reg;
    bool reg_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (reg_ref ? " @" : " ") << reg;
    }

    string svalue = string(addr);
    addr += svalue.size()+1;

    if (debug) {
        cout << ' ' << str::enquote(svalue) << endl;
    }

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new String(svalue));

    return addr;
}
