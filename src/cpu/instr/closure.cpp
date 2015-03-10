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

    Closure* clsr = new Closure(call_name);
    clsr.registers = uregisters;
    clsr.references = ureferences;
    clsr.registers_size = uregisters_size;

    for (int i = 0; i < uregisters_size; ++i) { ureferences[i] = true; }

    return addr;
}

byte* CPU::clframe(byte* addr) {
    /** Create new closure call frame.
     */
    int arguments;
    bool arguments_ref = false;

    arguments_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    arguments = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (debug) {
        cout << (arguments_ref ? " @" : " ") << arguments;
    }

    if (arguments_ref) {
        arguments = static_cast<Integer*>(fetch(arguments))->value();
    }

    if (frame_new != 0) { throw "requested new frame while last one is unused"; }
    frame_new = new ClosureFrame(0, arguments);

    return addr;
}
