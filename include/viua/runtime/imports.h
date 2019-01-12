/*
 *  Copyright (C) 2019 Marek Marecki
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

#ifndef VIUA_RUNTIME_IMPORTS_H
#define VIUA_RUNTIME_IMPORTS_H

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace viua { namespace runtime { namespace imports {
enum class Module_type {
    Native,     /*
                 * Calls to functions defined by native modules are handled by
                 * the FFI scheduler. They are not preemptible and thus have to
                 * be run on a separate thread.
                 */
    Bytecode,   /*
                 * Calls to functions defined by bytecode modules are handled by
                 * the virtual process scheduler, and are preemptible.
                 */
};

auto read_viua_library_path() -> std::vector<std::string>;
auto find_module(std::string const&) -> std::optional<std::pair<Module_type, std::string>>;
}}}

#endif
