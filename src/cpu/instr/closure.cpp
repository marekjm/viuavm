#include <iostream>
#include "../../bytecode/bytetypedef.h"
#include "../../types/type.h"
#include "../../types/integer.h"
#include "../../types/function.h"
#include "../../types/closure.h"
#include "../../support/pointer.h"
#include "../../exceptions.h"
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
    /** Create a closure from a function.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string call_name = string(addr);
    addr += (call_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    if (uregset != frames.back()->regset) {
        throw new Exception("creating closures from nonlocal registers is forbidden, go rethink your behaviour");
    }

    Closure* clsr = new Closure();
    clsr->function_name = call_name;
    clsr->regset = new RegisterSet(uregset->size());

    for (unsigned i = 0; i < uregset->size(); ++i) {
        // we must not mark empty registers as references or
        // segfaults will follow as CPU will try to update objects they are referring to, and
        // that's obviously no good
        // also, we shouldn't copy them
        if (uregset->at(i) == 0) { continue; }

        if (uregset->isflagged(i, BIND)) {
            uregset->unflag(i, BIND);
            clsr->regset->set(i, uregset->get(i));
            clsr->regset->flag(i, REFERENCE);
            uregset->flag(i, BOUND);
        }
    }

    place(reg, clsr);

    return addr;
}

byte* CPU::function(byte* addr) {
    /** Create function object in a register.
     *
     *  Such objects can be used to call functions, and
     *  are can be used to pass functions as parameters and
     *  return them from other functions.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string call_name = string(addr);
    addr += (call_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    Function* fn = new Function();
    fn->function_name = call_name;

    place(reg, fn);

    return addr;
}

byte* CPU::fcall(byte* addr) {
    /*  Call a function object.
     */
    int fn_reg;
    bool fn_reg_ref;

    fn_reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    fn_reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (fn_reg_ref) {
        fn_reg = static_cast<Integer*>(fetch(fn_reg))->value();
    }

    // FIXME: there should be a check it this is *really* a function object
    Function* fn = static_cast<Function*>(fetch(fn_reg));
    byte* call_address = bytecode+function_addresses.at(fn->name());

    // save return address for frame
    byte* return_address = (addr + sizeof(bool) + sizeof(int));

    if (frame_new == 0) {
        throw new Exception("fcall without a frame: use `clframe 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = fn->function_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = *(bool*)addr;
    pointer::inc<bool, byte>(addr);
    frame_new->place_return_value_in = *(int*)addr;

    pushFrame();

    if (fn->type() == "Closure") {
        uregset = dynamic_cast<Closure*>(fn)->regset;
    }

    return call_address;
}
