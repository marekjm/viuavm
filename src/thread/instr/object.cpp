#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/object.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* Thread::vmnew(byte* addr) {
    /** Create new instance of specified class.
     */
    int reg;
    bool reg_ref;

    viua::cpu::util::extractIntegerOperand(addr, reg_ref, reg);

    string class_name = string(addr);
    addr += (class_name.size()+1);

    if (reg_ref) {
        reg = static_cast<Integer*>(fetch(reg))->value();
    }

    if (cpu->typesystem.count(class_name) == 0) {
        throw new Exception("cannot create new instance of unregistered type: " + class_name);
    }

    place(reg, new Object(class_name));

    return addr;
}

byte* Thread::vmmsg(byte* addr) {
    /** Send a message to an object.
     *
     *  This instruction is used to perform a method call on an object using dynamic dispatch.
     *  To call a method using static dispatch (where a correct function is resolved during compilation) use
     *  "call" instruction.
     */
    int return_register_index;
    bool return_register_ref = false;

    viua::cpu::util::extractIntegerOperand(addr, return_register_ref, return_register_index);

    if (return_register_ref) {
        return_register_index = static_cast<Integer*>(fetch(return_register_index))->value();
    }

    string method_name = string(addr);

    Type* obj = frame_new->args->at(0);
    vector<string> mro = cpu->inheritanceChainOf(obj->type());
    mro.insert(mro.begin(), obj->type());

    string function_name = "";
    for (unsigned i = 0; i < mro.size(); ++i) {
        if (cpu->typesystem.at(mro[i])->accepts(method_name)) {
            function_name = cpu->typesystem.at(mro[i])->resolvesTo(method_name);
            break;
        }
    }
    if (function_name.size() == 0) {
        throw new Exception("class '" + obj->type() + "' does not accept method '" + method_name + "'");
    }

    bool is_native = (cpu->function_addresses.count(function_name) or cpu->linked_functions.count(function_name));
    bool is_foreign = cpu->foreign_functions.count(function_name);
    bool is_foreign_method = cpu->foreign_methods.count(function_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new Exception("method '" + method_name + "' resolves to undefined function '" + function_name + "' on class '" + obj->type() + "'");
    }

    if (is_foreign_method) {
        return callForeignMethod(addr, obj, function_name, return_register_ref, return_register_index, method_name);
    }

    auto caller = (is_native ? &Thread::callNative : &Thread::callForeign);
    return (this->*caller)(addr, function_name, return_register_ref, return_register_index, method_name);
}
