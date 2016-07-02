#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/prototype.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/operand.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::opclass(byte* addr) {
    /** Create a class.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    place(target, new Prototype(viua::operand::extractString(addr)));
    return addr;
}

byte* Process::opderive(byte* addr) {
    /** Push an ancestor class to prototype's inheritance chain.
     */
    Type* target = viua::operand::extract(addr)->resolve(this);

    string class_name = viua::operand::extractString(addr);

    if (not scheduler->isClass(class_name)) {
        throw new Exception("cannot derive from unregistered type: " + class_name);
    }

    static_cast<Prototype*>(target)->derive(class_name);

    return addr;
}

byte* Process::opattach(byte* addr) {
    /** Attach a function to a prototype as a method.
     */
    Type* target = viua::operand::extract(addr)->resolve(this);

    string function_name = viua::operand::extractString(addr);
    string method_name = viua::operand::extractString(addr);

    Prototype* proto = static_cast<Prototype*>(target);

    if (not (scheduler->isNativeFunction(function_name) or scheduler->isForeignFunction(function_name))) {
        throw new Exception("cannot attach undefined function '" + function_name + "' as a method '" + method_name + "' of prototype '" + proto->getTypeName() + "'");
    }

    proto->attach(function_name, method_name);

    return addr;
}

byte* Process::opregister(byte* addr) {
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
    scheduler->cpu()->typesystem[new_proto->getTypeName()] = new_proto;
    uregset->empty(reg);

    return addr;
}
