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


void viua::front::vm::initialise(viua::kernel::Kernel& kernel,
                                 std::string const& program,
                                 std::vector<std::string> args) {
    auto loader = Loader{program};
    loader.executable();

    for (auto const& each : loader.dynamic_imports()) {
        kernel.load_module(each);
    }

    auto const bytes = loader.get_bytecode_size();
    auto bytecode    = loader.get_bytecode();

    auto const function_address_mapping = loader.get_function_addresses();
    for (auto const& p : function_address_mapping) {
        kernel.mapfunction(p.first, p.second);
    }
    for (auto const& p : loader.get_block_addresses()) {
        kernel.mapblock(p.first, p.second);
    }

    kernel.commandline_arguments = args;

    kernel.load(std::move(bytecode)).bytes(bytes);
}

void viua::front::vm::preload_libraries(viua::kernel::Kernel& kernel) {
    /** This method preloads dynamic libraries specified by environment.
     */
    auto const module_to_preload = support::env::get_paths("VIUA_IMPORT");
    for (auto const& each : module_to_preload) {
        kernel.load_module(each);
    }
}
