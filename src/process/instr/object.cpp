#include <iostream>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/types/object.h>
#include <viua/types/string.h>
#include <viua/cpu/opex.h>
#include <viua/exceptions.h>
#include <viua/cpu/registerset.h>
#include <viua/operand.h>
#include <viua/assert.h>
#include <viua/cpu/cpu.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* Process::opnew(byte* addr) {
    /** Create new instance of specified class.
     */
    unsigned target = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    string class_name = viua::operand::extractString(addr);

    if (not scheduler->isClass(class_name)) {
        throw new Exception("cannot create new instance of unregistered type: " + class_name);
    }

    place(target, new Object(class_name));

    return addr;
}

byte* Process::opmsg(byte* addr) {
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

    string method_name = viua::operand::extractString(addr);

    Type* obj = frame_new->args->at(0);
    if (Pointer* ptr = dynamic_cast<Pointer*>(obj)) {
        obj = ptr->to();
    }
    if (not scheduler->isClass(obj->type())) {
        throw new Exception("unregistered type cannot be used for dynamic dispatch: " + obj->type());
    }
    vector<string> mro = scheduler->inheritanceChainOf(obj->type());
    mro.insert(mro.begin(), obj->type());

    string function_name = "";
    for (unsigned i = 0; i < mro.size(); ++i) {
        if (scheduler->cpu()->typesystem.count(mro[i]) == 0) {
            throw new Exception("unavailable base type in inheritance hierarchy of " + mro[0] + ": " + mro[i]);
        }
        if (scheduler->cpu()->typesystem.at(mro[i])->accepts(method_name)) {
            function_name = scheduler->cpu()->typesystem.at(mro[i])->resolvesTo(method_name);
            break;
        }
    }
    if (function_name.size() == 0) {
        throw new Exception("class '" + obj->type() + "' does not accept method '" + method_name + "'");
    }

    bool is_native = (scheduler->cpu()->function_addresses.count(function_name) or scheduler->cpu()->linked_functions.count(function_name));
    bool is_foreign = scheduler->isForeignFunction(function_name);
    bool is_foreign_method = scheduler->isForeignMethod(function_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new Exception("method '" + method_name + "' resolves to undefined function '" + function_name + "' on class '" + obj->type() + "'");
    }

    if (is_foreign_method) {
        return callForeignMethod(addr, obj, function_name, return_register_ref, return_register_index, method_name);
    }

    auto caller = (is_native ? &Process::callNative : &Process::callForeign);
    return (this->*caller)(addr, function_name, return_register_ref, return_register_index, method_name);
}

byte* Process::opinsert(byte* addr) {
    /** Insert an object as an attribute of another object.
     */
    Type* object_operand = viua::operand::extract(addr)->resolve(this);
    Type* key_operand = viua::operand::extract(addr)->resolve(this);
    unsigned source_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    viua::assertions::assert_implements<Object>(object_operand, "Object");
    viua::assertions::assert_typeof(key_operand, "String");

    static_cast<Object*>(object_operand)->insert(static_cast<String*>(key_operand)->str(), pop(source_index));

    return addr;
}

byte* Process::opremove(byte* addr) {
    /** Remove an attribute of another object.
     */
    unsigned target_index = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);
    Type* object_operand = viua::operand::extract(addr)->resolve(this);
    Type* key_operand = viua::operand::extract(addr)->resolve(this);

    viua::assertions::assert_implements<Object>(object_operand, "Object");
    viua::assertions::assert_typeof(key_operand, "String");

    place(target_index, static_cast<Object*>(object_operand)->remove(static_cast<String*>(key_operand)->str()));

    return addr;
}
