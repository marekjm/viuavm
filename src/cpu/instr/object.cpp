#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/object.h>
#include <viua/support/pointer.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/cpu/cpu.h>
using namespace std;


byte* CPU::vmnew(byte* addr) {
    /** Create new instance of specified class.
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
        throw new Exception("cannot create new instance of unregistered type: " + class_name);
    }

    place(reg, new Object(class_name));

    return addr;
}

byte* CPU::vmmsg(byte* addr) {
    /** Send a message to an object.
     *
     *  This instruction is used to perform a method call on an object using dynamic dispatch.
     *  To call a method using static dispatch (where a correct function is resolved during compilation) use
     *  "call" instruction.
     */
    int return_register_index, object_operand_index;
    bool return_register_ref = false, object_operand_ref = false;

    return_register_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    return_register_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    object_operand_ref = *((bool*)addr);
    pointer::inc<bool, byte>(addr);
    object_operand_index = *((int*)addr);
    pointer::inc<int, byte>(addr);

    if (return_register_ref) {
        return_register_index = static_cast<Integer*>(fetch(return_register_index))->value();
    }
    if (object_operand_ref) {
        object_operand_index = static_cast<Integer*>(fetch(object_operand_index))->value();
    }

    string method_name = string(addr);

    Object* obj = static_cast<Object*>(fetch(object_operand_index));
    vector<string> mro = inheritanceChainOf(obj->type());
    mro.insert(mro.begin(), obj->type());

    string function_name = "";
    for (unsigned i = 0; i < mro.size(); ++i) {
        if (typesystem.at(mro[i])->accepts(method_name)) {
            function_name = typesystem.at(mro[i])->resolvesTo(method_name);
            break;
        }
    }
    if (function_name.size() == 0) {
        throw new Exception("class '" + obj->type() + "' does not accept method '" + method_name + "'");
    }

    bool is_native = (function_addresses.count(function_name) or linked_functions.count(function_name));
    bool is_foreign = foreign_functions.count(function_name);
    bool is_foreign_method = foreign_methods.count(function_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new Exception("method '" + method_name + "' resolves to undefined function '" + function_name + "' on class '" + obj->type() + "'");
    }

    if (is_foreign_method) {
        return callForeignMethod(addr, obj, function_name, return_register_ref, return_register_index, method_name);
    }

    auto caller = (is_native ? &CPU::callNative : &CPU::callForeign);
    return (this->*caller)(addr, function_name, return_register_ref, return_register_index, method_name);

    return addr;
}
