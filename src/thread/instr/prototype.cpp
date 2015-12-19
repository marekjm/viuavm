#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/prototype.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::vmclass(byte* addr) {
    /** Create a class.
     */
    int target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    string class_name = string(addr);
    addr += (class_name.size()+1);

    place(target, new Prototype(class_name));

    return addr;
}

byte* Thread::vmderive(byte* addr) {
    /** Push an ancestor class to prototype's inheritance chain.
     */
    Type* target = viua::operand::extract(addr)->resolve(this);

    string class_name = string(addr);
    addr += (class_name.size()+1);

    if (cpu->typesystem.count(class_name) == 0) {
        throw new Exception("cannot derive from unregistered type: " + class_name);
    }

    static_cast<Prototype*>(target)->derive(class_name);

    return addr;
}

byte* Thread::vmattach(byte* addr) {
    /** Attach a function to a prototype as a method.
     */
    int reg;
    bool reg_ref;

    viua::cpu::util::extractIntegerOperand(addr, reg_ref, reg);

    string function_name = string(addr);
    addr += (function_name.size()+1);

    string method_name = string(addr);
    addr += (method_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    Prototype* proto = static_cast<Prototype*>(fetch(reg));

    if (not (cpu->function_addresses.count(function_name) or cpu->linked_functions.count(function_name) or cpu->foreign_functions.count(function_name))) {
        throw new Exception("cannot attach undefined function '" + function_name + "' as a method '" + method_name + "' of prototype '" + proto->getTypeName() + "'");
    }

    proto->attach(function_name, method_name);

    return addr;
}

byte* Thread::vmregister(byte* addr) {
    /** Register a prototype in the typesystem.
     */
    int reg;
    bool reg_ref;

    viua::cpu::util::extractIntegerOperand(addr, reg_ref, reg);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    Prototype* new_proto = static_cast<Prototype*>(fetch(reg));
    // FIXME: mutex
    cpu->typesystem[new_proto->getTypeName()] = new_proto;
    uregset->empty(reg);

    return addr;
}
