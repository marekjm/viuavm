#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/closure.h"
#include "../../support/pointer.h"
#include "../cpu.h"
using namespace std;


byte* CPU::closure(byte* addr) {
    /*  Call a closure.
     */
    string call_name = string(addr);
    byte* call_address = bytecode+function_addresses.at(call_name);
    addr += call_name.size();

    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    if (debug) {
        cout << " for function '" << call_name << "' in register " << reg;
    }

    if (uregisters != frames.back()->registers) {
        throw "creating closures from nonlocal registers is forbidden, go rethink your behaviour";
    }

    Closure* clsr = new Closure();
    clsr.registers = uregisters;
    clsr.references = ureferences;
    clsr.registers_size = uregisters_size;

    for (int i = 0; i < uregisters_size; ++i) { ureferences[i] = true; }

    return addr;
}
