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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/integer.h>
#include <viua/types/prototype.h>
#include <viua/exceptions.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


byte* viua::process::Process::opclass(byte* addr) {
    /** Create a class.
     */
    unsigned target = 0;
    string class_name;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, class_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    place(target, new viua::types::Prototype(class_name));

    return addr;
}

byte* viua::process::Process::opderive(byte* addr) {
    /** Push an ancestor class to prototype's inheritance chain.
     */
    viua::types::Type* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    string class_name;
    tie(addr, class_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->isClass(class_name)) {
        throw new viua::types::Exception("cannot derive from unregistered type: " + class_name);
    }

    static_cast<viua::types::Prototype*>(target)->derive(class_name);

    return addr;
}

byte* viua::process::Process::opattach(byte* addr) {
    /** Attach a function to a prototype as a method.
     */
    viua::types::Type* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    string function_name, method_name;
    tie(addr, function_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);
    tie(addr, method_name) = viua::bytecode::decoder::operands::fetch_atom(addr, this);

    viua::types::Prototype* proto = static_cast<viua::types::Prototype*>(target);

    if (not (scheduler->isNativeFunction(function_name) or scheduler->isForeignFunction(function_name))) {
        throw new viua::types::Exception("cannot attach undefined function '" + function_name + "' as a method '" + method_name + "' of prototype '" + proto->getTypeName() + "'");
    }

    proto->attach(function_name, method_name);

    return addr;
}

byte* viua::process::Process::opregister(byte* addr) {
    /** Register a prototype in the typesystem.
     */
    unsigned source = 0;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    unique_ptr<viua::types::Prototype> prototype {static_cast<viua::types::Prototype*>(pop(source).release())};
    scheduler->registerPrototype(std::move(prototype));

    return addr;
}
