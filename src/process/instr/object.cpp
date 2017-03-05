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
#include <viua/types/function.h>
#include <viua/types/closure.h>
#include <viua/exceptions.h>
#include <viua/kernel/registerset.h>
#include <viua/assert.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opnew(viua::internals::types::byte* addr) {
    /** Create new instance of specified class.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    string class_name;
    tie(addr, class_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isClass(class_name)) {
        throw new viua::types::Exception("cannot create new instance of unregistered type: " + class_name);
    }

    *target = unique_ptr<viua::types::Type>{new viua::types::Object(class_name)};

    return addr;
}

viua::internals::types::byte* viua::process::Process::opmsg(viua::internals::types::byte* addr) {
    /** Send a message to an object.
     *
     *  This instruction is used to perform a method call on an object using dynamic dispatch.
     *  To call a method using static dispatch (where a correct function is resolved during compilation) use
     *  "call" instruction.
     */
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* return_register = nullptr;

    if (not return_void) {
        tie(addr, return_register) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    string method_name;
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Type* fn_source = nullptr;
        tie(addr, fn_source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

        auto fn = dynamic_cast<viua::types::Function*>(fn_source);
        if (not fn) {
            throw new viua::types::Exception("type is not callable: " + fn_source->type());
        }
        method_name = fn->name();

        if (fn->type() == "Closure") {
            stack.frame_new->setLocalRegisterSet(static_cast<viua::types::Closure*>(fn)->rs(), false);
        }
    } else {
        tie(addr, method_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

    auto obj = stack.frame_new->arguments->at(0);
    if (auto ptr = dynamic_cast<viua::types::Pointer*>(obj)) {
        obj = ptr->to();
    }
    if (not scheduler->isClass(obj->type())) {
        throw new viua::types::Exception("unregistered type cannot be used for dynamic dispatch: " + obj->type());
    }
    vector<string> mro = scheduler->inheritanceChainOf(obj->type());
    mro.insert(mro.begin(), obj->type());

    string function_name = "";
    for (decltype(mro.size()) i = 0; i < mro.size(); ++i) {
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
        return callForeignMethod(addr, obj, function_name, return_register, method_name);
    }

    auto caller = (is_native ? &viua::process::Process::callNative : &viua::process::Process::callForeign);
    return (this->*caller)(addr, function_name, return_register, method_name);
}

viua::internals::types::byte* viua::process::Process::opinsert(viua::internals::types::byte* addr) {
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
        static_cast<viua::types::Object*>(object_operand)->insert(static_cast<viua::types::String*>(key_operand)->str(), source->copy());
    } else {
        viua::kernel::Register* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);
        static_cast<viua::types::Object*>(object_operand)->insert(static_cast<viua::types::String*>(key_operand)->str(), source->give());
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opremove(viua::internals::types::byte* addr) {
    /** Remove an attribute of another object.
     */
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Type *object_operand = nullptr, *key_operand = nullptr;
    tie(addr, object_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, key_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::assert_implements<viua::types::Object>(object_operand, "viua::types::Object");
    viua::assertions::assert_typeof(key_operand, "String");

    unique_ptr<viua::types::Type> result { static_cast<viua::types::Object*>(object_operand)->remove(static_cast<viua::types::String*>(key_operand)->str()) };
    if (not void_target) {
        *target = std::move(result);
    }

    return addr;
}
