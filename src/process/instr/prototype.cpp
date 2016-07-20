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
    unsigned source = viua::operand::getRegisterIndex(viua::operand::extract(addr).get(), this);

    Prototype* new_proto = static_cast<Prototype*>(fetch(source));
    scheduler->registerPrototype(new_proto);
    uregset->empty(source);

    return addr;
}
