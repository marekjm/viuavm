#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/function.h>
#include <viua/types/closure.h>
#include <viua/types/reference.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Process::openclose(byte* addr) {
    /** Enclose object by reference.
     */
    Closure *target_closure = static_cast<Closure*>(viua::operand::extract(addr)->resolve(this));
    unsigned target_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (target_register >= target_closure->regset->size()) {
        throw new Exception("cannot enclose object: register index out exceeded size of closure register set");
    }

    unsigned source_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    Type* enclosed_object = uregset->at(source_register);
    Reference *rf = dynamic_cast<Reference*>(enclosed_object);
    if (rf == nullptr) {
        // turn enclosed object into a reference to take it out of VM's default
        // memory management scheme and put it under reference-counting scheme
        // this is needed to bind the enclosed object's life to lifetime of the closure
        rf = new Reference(enclosed_object);
        uregset->empty(source_register);    // empty - do not delete the enclosed object or SEGFAULTS will follow
        uregset->set(source_register, rf);  // set the register to contain the newly-created reference
    }
    target_closure->regset->set(target_register, rf->copy());

    return addr;
}

byte* Process::openclosecopy(byte* addr) {
    /** Enclose object by copy.
     */
    Closure *target_closure = static_cast<Closure*>(viua::operand::extract(addr)->resolve(this));
    unsigned target_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (target_register >= target_closure->regset->size()) {
        throw new Exception("cannot enclose object: register index out exceeded size of closure register set");
    }

    target_closure->regset->set(target_register, viua::operand::extract(addr)->resolve(this)->copy());

    return addr;
}

byte* Process::openclosemove(byte* addr) {
    /** Enclose object by move.
     */
    Closure *target_closure = static_cast<Closure*>(viua::operand::extract(addr)->resolve(this));
    unsigned target_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    if (target_register >= target_closure->regset->size()) {
        throw new Exception("cannot enclose object: register index out exceeded size of closure register set");
    }

    unsigned source_register = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    target_closure->regset->set(target_register, uregset->pop(source_register));

    return addr;
}

byte* Process::opclosure(byte* addr) {
    /** Create a closure from a function.
     */
    if (uregset != frames.back()->regset) {
        throw new Exception("creating closures from nonlocal registers is forbidden, go rethink your behaviour");
    }

    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string call_name = viua::operand::extractString(addr);

    Closure* clsr = new Closure();
    clsr->function_name = call_name;
    clsr->regset = new RegisterSet(uregset->size());

    place(target, clsr);

    return addr;
}

byte* Process::opfunction(byte* addr) {
    /** Create function object in a register.
     *
     *  Such objects can be used to call functions, and
     *  are can be used to pass functions as parameters and
     *  return them from other functions.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string call_name = viua::operand::extractString(addr);

    Function* fn = new Function();
    fn->function_name = call_name;

    place(target, fn);

    return addr;
}

byte* Process::opfcall(byte* addr) {
    /*  Call a function object.
     */
    int return_value_reg;
    bool return_value_ref;
    viua::cpu::util::extractIntegerOperand(addr, return_value_ref, return_value_reg);

    unsigned fn_reg = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    // FIXME: there should be a check it this is *really* a function object
    Function* fn = static_cast<Function*>(fetch(fn_reg));

    string call_name = fn->name();
    bool function_found = (cpu->function_addresses.count(call_name) or cpu->linked_functions.count(call_name));

    if (not function_found) {
        throw new Exception("fcall to undefined function: " + call_name);
    }

    byte* call_address = nullptr;
    if (cpu->function_addresses.count(call_name)) {
        call_address = cpu->bytecode+cpu->function_addresses.at(call_name);
        jump_base = cpu->bytecode;
    } else {
        call_address = cpu->linked_functions.at(call_name).second;
        jump_base = cpu->linked_modules.at(cpu->linked_functions.at(call_name).first).second;
    }

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("fcall without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_value_ref;
    frame_new->place_return_value_in = static_cast<unsigned>(return_value_reg);

    if (fn->type() == "Closure") {
        frame_new->setLocalRegisterSet(static_cast<Closure*>(fn)->regset, false);
    }

    pushFrame();

    return call_address;
}
