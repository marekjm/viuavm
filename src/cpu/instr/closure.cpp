#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/object.h"
#include "../../types/integer.h"
#include "../../types/closure.h"
#include "../../support/pointer.h"
#include "../registerset.h"
#include "../cpu.h"
using namespace std;


byte* CPU::clbind(byte* addr) {
    /** Mark a register to be bound by next closure.
     *
     *  After next closure instruction, the BIND mask is removed and
     *  BOUND mask is inserted to hint the CPU that this register
     *  contains an object bound outside of its immediate scope.
     *  Objects are not freed from registers marked as BOUND.
     */
    int a;
    bool ref = false;

    ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    a = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (ref) {
        a = static_cast<Integer*>(fetch(a))->value();
    }

    uregset->flag(a, BIND);

    return addr;
}

byte* CPU::closure(byte* addr) {
    /*  Call a closure.
     */
    string call_name = string(addr);
    addr += call_name.size();

    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (reg_ref) {
        cout << "here?" << endl;
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    if (debug) {
        cout << " for function '" << call_name << "' in register " << reg;
    }

    if (uregset != frames.back()->regset) {
        throw "creating closures from nonlocal registers is forbidden, go rethink your behaviour";
    }

    Closure* clsr = new Closure();
    clsr->function_name = call_name;
    cout << '\n';
    cout << "closure registerset size: " << uregset->size() << endl;
    clsr->regset = new RegisterSet(uregset->size());

    for (unsigned i = 0; i < uregset->size(); ++i) {
        // we must not mark empty registers as references or
        // segfaults will follow as CPU will try to update objects they are referring to, and
        // that's obviously no good
        // also, we shouldn't copy them
        if (uregset->at(i) == 0) { continue; }

        // do not copy the closure into its own environment
        if (i != unsigned(reg)) {
            cout << "binding: " << uregset->get(i)->type() << " at " << hex << uregset->get(i) << dec << endl;
            clsr->regset->set(i, uregset->get(i));
            clsr->regset->unflag(i, KEEP);
            clsr->regset->flag(i, REFERENCE);
        }

        uregset->flag(i, KEEP);
    }

    place(reg, clsr);

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
    frame_new = new Frame(0, arguments, 0);

    return addr;
}

byte* CPU::clcall(byte* addr) {
    /*  Call a closure.
     */
    int closure_reg;
    bool closure_reg_ref;

    closure_reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    closure_reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (closure_reg_ref) {
        closure_reg = static_cast<Integer*>(fetch(closure_reg))->value();
    }

    // FIXME: there should be a check it this is *really* a closure object
    Closure* clsr = static_cast<Closure*>(fetch(closure_reg));
    byte* call_address = bytecode+function_addresses.at(clsr->function_name);

    // save return address for frame
    byte* return_address = (addr + sizeof(bool) + sizeof(int));

    if (debug) {
        cout << " '" << clsr->function_name << "' " << (long)(call_address-bytecode);
        cout << ": setting return address to bytecode " << (long)(return_address-bytecode);
    }
    if (frame_new == 0) {
        throw "closure call without a frame: use `clframe 0' in source code if the closure takes no parameters";
    }
    // set function name and return address
    frame_new->function_name = clsr->function_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = *(bool*)addr;
    pointer::inc<bool, byte>(addr);
    frame_new->place_return_value_in = *(int*)addr;
    if (debug) {
        if (frame_new->place_return_value_in == 0) {
            cout << " (return value will be discarded)";
        } else {
            cout << " (return value will be placed in: " << (frame_new->resolve_return_value_register ? "@" : "") << frame_new->place_return_value_in << ')';
        }
    }

    // adjust register set
    uregset = clsr->regset;

    // FIXME: remove this print
    //cout << "\npushing new frame on stack: " << hex << frame_new << dec << " (for function: " << frame_new->function_name << ')' << endl;
    // use frame for function call
    frames.push_back(frame_new);

    // and free the hook
    frame_new = 0;

    return call_address;
}
