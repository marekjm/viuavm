/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/types/object.h>
#include <viua/types/string.h>
#include <viua/exceptions.h>
#include <viua/kernel/registerset.h>
#include <viua/assert.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* viua::process::Process::opnew(byte* addr) {
    /** Create new instance of specified class.
     */
    unsigned target = 0;
    string class_name;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, class_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isClass(class_name)) {
        throw new viua::types::Exception("cannot create new instance of unregistered type: " + class_name);
    }

    place(target, unique_ptr<viua::types::Type>{new viua::types::Object(class_name)});

    return addr;
}

byte* viua::process::Process::opmsg(byte* addr) {
    /** Send a message to an object.
     *
     *  This instruction is used to perform a method call on an object using dynamic dispatch.
     *  To call a method using static dispatch (where a correct function is resolved during compilation) use
     *  "call" instruction.
     */
    unsigned return_register = 0;
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);

    if (not return_void) {
        tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    string method_name;
    tie(addr, method_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    auto obj = frame_new->args->at(0);
    if (auto ptr = dynamic_cast<viua::types::Pointer*>(obj)) {
        obj = ptr->to();
    }
    if (not scheduler->isClass(obj->type())) {
        throw new viua::types::Exception("unregistered type cannot be used for dynamic dispatch: " + obj->type());
    }
    vector<string> mro = scheduler->inheritanceChainOf(obj->type());
    mro.insert(mro.begin(), obj->type());

    string function_name = "";
    for (unsigned i = 0; i < mro.size(); ++i) {
        if (not scheduler->isClass(mro[i])) {
            throw new viua::types::Exception("unavailable base type in inheritance hierarchy of " + mro[0] + ": " + mro[i]);
        }
        if (scheduler->classAccepts(mro[i], method_name)) {
            function_name = scheduler->resolveMethodName(mro[i], method_name);
            break;
        }
    }
    if (function_name.size() == 0) {
        throw new viua::types::Exception("class '" + obj->type() + "' does not accept method '" + method_name + "'");
    }

    bool is_native = scheduler->isNativeFunction(function_name);
    bool is_foreign = scheduler->isForeignFunction(function_name);
    bool is_foreign_method = scheduler->isForeignMethod(function_name);

    if (not (is_native or is_foreign or is_foreign_method)) {
        throw new viua::types::Exception("method '" + method_name + "' resolves to undefined function '" + function_name + "' on class '" + obj->type() + "'");
    }

    if (is_foreign_method) {
        // FIXME: remove the need for static_cast<>
        // the cast is safe because register indexes cannot be negative, but it looks ugly
        return callForeignMethod(addr, obj, function_name, return_void, return_register, method_name);
    }

    auto caller = (is_native ? &viua::process::Process::callNative : &viua::process::Process::callForeign);
    // FIXME: remove the need for static_cast<>
    // the cast is safe because register indexes cannot be negative, but it looks ugly
    return (this->*caller)(addr, function_name, return_void, return_register, method_name);
}

byte* viua::process::Process::opinsert(byte* addr) {
    /** Insert an object as an attribute of another object.
     */
    viua::types::Type *object_operand = nullptr, *key_operand = nullptr;
    tie(addr, object_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, key_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    viua::assertions::assert_implements<viua::types::Object>(object_operand, "viua::types::Object");
    viua::assertions::assert_typeof(key_operand, "String");

    if (viua::bytecode::decoder::operands::get_operand_type(addr) == OT_POINTER) {
        viua::types::Type* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);
        static_cast<viua::types::Object*>(object_operand)->insert(static_cast<viua::types::String*>(key_operand)->str(), std::move(source->copy()));
    } else {
        unsigned source_index = 0;
        tie(addr, source_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
        static_cast<viua::types::Object*>(object_operand)->insert(static_cast<viua::types::String*>(key_operand)->str(), std::move(pop(source_index)));
    }

    return addr;
}

byte* viua::process::Process::opremove(byte* addr) {
    /** Remove an attribute of another object.
     */
    unsigned target_index = 0;
    bool void_target = false;
    if (viua::bytecode::decoder::operands::get_operand_type(addr) == OT_VOID) {
        void_target = true;
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    } else {
        tie(addr, target_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    }

    viua::types::Type *object_operand = nullptr, *key_operand = nullptr;
    tie(addr, object_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, key_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::assert_implements<viua::types::Object>(object_operand, "viua::types::Object");
    viua::assertions::assert_typeof(key_operand, "String");

    unique_ptr<viua::types::Type> result { static_cast<viua::types::Object*>(object_operand)->remove(static_cast<viua::types::String*>(key_operand)->str()) };
    if (not void_target) {
        place(target_index, std::move(result));
    }

    return addr;
}
