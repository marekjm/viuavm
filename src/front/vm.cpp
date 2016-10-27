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

#include <viua/front/vm.h>
using namespace std;


void viua::front::vm::initialise(Kernel *kernel, const string& program, vector<string> args) {
    Loader loader(program);
    loader.executable();

    uint64_t bytes = loader.getBytecodeSize();
    byte* bytecode = loader.getBytecode();

    map<string, uint64_t> function_address_mapping = loader.getFunctionAddresses();
    for (auto p : function_address_mapping) { kernel->mapfunction(p.first, p.second); }
    for (auto p : loader.getBlockAddresses()) { kernel->mapblock(p.first, p.second); }

    kernel->commandline_arguments = args;

    kernel->load(bytecode).bytes(bytes);
}

void viua::front::vm::load_standard_prototypes(Kernel* kernel) {
    Prototype* proto_object = new Prototype("Object");
    kernel->registerForeignPrototype("Object", proto_object);

    Prototype* proto_string = new Prototype("String");
    proto_string->attach("String::stringify/2", "stringify/2");
    proto_string->attach("String::represent/2", "represent/2");
    proto_string->attach("String::startswith/2", "startswith/2");
    proto_string->attach("String::endswith/2", "endswith/2");
    proto_string->attach("String::format/", "format/");  // FIXME: fixed-arity, two- and three-parameter versions
    proto_string->attach("String::substr/", "substr/");  // FIXME: fixed-arity versions (2, 3 and 4 parameters)
    proto_string->attach("String::concatenate/2", "concatenate/2");
    proto_string->attach("String::join/1", "join/1");
    proto_string->attach("String::size/1", "size/1");
    kernel->registerForeignPrototype("String", proto_string);
    kernel->registerForeignMethod("String::stringify/2", static_cast<ForeignMethodMemberPointer>(&String::stringify));
    kernel->registerForeignMethod("String::represent/2", static_cast<ForeignMethodMemberPointer>(&String::represent));
    kernel->registerForeignMethod("String::startswith/2", static_cast<ForeignMethodMemberPointer>(&String::startswith));
    kernel->registerForeignMethod("String::endswith/2", static_cast<ForeignMethodMemberPointer>(&String::endswith));
    kernel->registerForeignMethod("String::format/", static_cast<ForeignMethodMemberPointer>(&String::format));
    kernel->registerForeignMethod("String::substr/", static_cast<ForeignMethodMemberPointer>(&String::substr));
    kernel->registerForeignMethod("String::concatenate/2", static_cast<ForeignMethodMemberPointer>(&String::concatenate));
    kernel->registerForeignMethod("String::join/", static_cast<ForeignMethodMemberPointer>(&String::join));
    kernel->registerForeignMethod("String::size/1", static_cast<ForeignMethodMemberPointer>(&String::size));

    Prototype* proto_process = new Prototype("Process");
    proto_process->attach("Process::joinable/1", "joinable/1");
    proto_process->attach("Process::detach/1", "detach/1");
    kernel->registerForeignPrototype("Process", proto_process);
    kernel->registerForeignMethod("Process::joinable/1", static_cast<ForeignMethodMemberPointer>(&ProcessType::joinable));
    kernel->registerForeignMethod("Process::detach/1", static_cast<ForeignMethodMemberPointer>(&ProcessType::detach));

    Prototype* proto_pointer = new Prototype("Pointer");
    proto_pointer->attach("Pointer::expired/1", "expired/1");
    kernel->registerForeignPrototype("Pointer", proto_pointer);
    kernel->registerForeignMethod("Pointer::expired/1", static_cast<ForeignMethodMemberPointer>(&Pointer::expired));
}

void viua::front::vm::preload_libraries(Kernel* kernel) {
    /** This method preloads dynamic libraries specified by environment.
     */
    vector<string> preload_native = support::env::getpaths("VIUAPRELINK");
    for (unsigned i = 0; i < preload_native.size(); ++i) {
        kernel->loadNativeLibrary(preload_native[i]);
    }

    vector<string> preload_foreign = support::env::getpaths("VIUAPREIMPORT");
    for (unsigned i = 0; i < preload_foreign.size(); ++i) {
        kernel->loadForeignLibrary(preload_foreign[i]);
    }
}
