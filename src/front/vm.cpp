/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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
#include <viua/front/vm.h>
using namespace std;


void viua::front::vm::initialise(viua::kernel::Kernel* kernel,
                                 const string& program,
                                 vector<string> args) {
    auto loader = Loader{program};
    loader.executable();

    auto const bytes = loader.get_bytecode_size();
    auto bytecode    = loader.get_bytecode();

    auto const function_address_mapping = loader.get_function_addresses();
    for (auto const p : function_address_mapping) {
        kernel->mapfunction(p.first, p.second);
    }
    for (auto const p : loader.get_block_addresses()) {
        kernel->mapblock(p.first, p.second);
    }

    kernel->commandline_arguments = args;

    kernel->load(std::move(bytecode)).bytes(bytes);
}

void viua::front::vm::load_standard_prototypes(viua::kernel::Kernel* kernel) {
    auto proto_object = make_unique<viua::types::Prototype>("Object");
    kernel->register_foreign_prototype("Object", std::move(proto_object));

    auto proto_string = make_unique<viua::types::Prototype>("String");
    proto_string->attach("String::stringify/2", "stringify/2");
    proto_string->attach("String::represent/2", "represent/2");
    proto_string->attach("String::startswith/2", "startswith/2");
    proto_string->attach("String::endswith/2", "endswith/2");
    proto_string->attach("String::format/",
                         "format/");  // FIXME: wrap-arity, two- and
                                      // three-parameter versions
    proto_string->attach("String::substr/", "substr/");  // FIXME: wrap-arity
                                                         // versions (2, 3 and 4
                                                         // parameters)
    proto_string->attach("String::concatenate/2", "concatenate/2");
    proto_string->attach("String::join/1", "join/1");
    proto_string->attach("String::size/1", "size/1");
    kernel->register_foreign_prototype("String", std::move(proto_string));
    kernel->register_foreign_method("String::stringify/2",
                                    static_cast<ForeignMethodMemberPointer>(
                                        &viua::types::String::stringify));
    kernel->register_foreign_method("String::represent/2",
                                    static_cast<ForeignMethodMemberPointer>(
                                        &viua::types::String::represent));
    kernel->register_foreign_method("String::startswith/2",
                                    static_cast<ForeignMethodMemberPointer>(
                                        &viua::types::String::startswith));
    kernel->register_foreign_method("String::endswith/2",
                                    static_cast<ForeignMethodMemberPointer>(
                                        &viua::types::String::endswith));
    kernel->register_foreign_method(
        "String::format/",
        static_cast<ForeignMethodMemberPointer>(&viua::types::String::format));
    kernel->register_foreign_method(
        "String::substr/",
        static_cast<ForeignMethodMemberPointer>(&viua::types::String::substr));
    kernel->register_foreign_method("String::concatenate/2",
                                    static_cast<ForeignMethodMemberPointer>(
                                        &viua::types::String::concatenate));
    kernel->register_foreign_method(
        "String::join/",
        static_cast<ForeignMethodMemberPointer>(&viua::types::String::join));
    kernel->register_foreign_method(
        "String::size/1",
        static_cast<ForeignMethodMemberPointer>(&viua::types::String::size));

    auto proto_process = make_unique<viua::types::Prototype>("Process");
    kernel->register_foreign_prototype("Process", std::move(proto_process));

    auto proto_pointer = make_unique<viua::types::Prototype>("Pointer");
    proto_pointer->attach("Pointer::expired/1", "expired/1");
    kernel->register_foreign_prototype("Pointer", std::move(proto_pointer));
    kernel->register_foreign_method("Pointer::expired/1",
                                    static_cast<ForeignMethodMemberPointer>(
                                        &viua::types::Pointer::expired));
}

void viua::front::vm::preload_libraries(viua::kernel::Kernel* kernel) {
    /** This method preloads dynamic libraries specified by environment.
     */
    auto const preload_native = support::env::get_paths("VIUAPRELINK");
    for (auto const& each : preload_native) {
        kernel->load_native_library(each);
    }

    auto const preload_foreign = support::env::get_paths("VIUAPREIMPORT");
    for (auto const& each : preload_foreign) {
        kernel->load_foreign_library(each);
    }
}
