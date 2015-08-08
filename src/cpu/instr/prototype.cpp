#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/prototype.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* CPU::vmclass(byte* addr) {
    /** Create a class.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string class_name = string(addr);
    addr += (class_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    place(reg, new Prototype(class_name));

    return addr;
}

byte* CPU::vmderive(byte* addr) {
    /** Push an ancestor class to prototype's inheritance chain.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string class_name = string(addr);
    addr += (class_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    if (typesystem.count(class_name) == 0) {
        throw new Exception("cannot derive from unregistered type: " + class_name);
    }

    static_cast<Prototype*>(fetch(reg))->derive(class_name);

    return addr;
}

byte* CPU::vmattach(byte* addr) {
    /** Attach a function to a prototype as a method.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    string function_name = string(addr);
    addr += (function_name.size()+1);

    string method_name = string(addr);
    addr += (method_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    Prototype* proto = static_cast<Prototype*>(fetch(reg));

    if (not (function_addresses.count(function_name) or linked_functions.count(function_name) or foreign_functions.count(function_name))) {
        throw new Exception("cannot attach undefined function '" + function_name + "' as a method '" + method_name + "' of prototype '" + proto->getTypeName() + "'");
    }

    proto->attach(function_name, method_name);

    return addr;
}

byte* CPU::vmregister(byte* addr) {
    /** Register a prototype in the typesystem.
     */
    int reg;
    bool reg_ref;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    reg = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    Prototype* new_proto = static_cast<Prototype*>(fetch(reg));
    typesystem[new_proto->getTypeName()] = new_proto;
    uregset->empty(reg);

    return addr;
}
