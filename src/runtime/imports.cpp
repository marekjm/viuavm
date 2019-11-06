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

#include <regex>
#include <sstream>
#include <viua/runtime/imports.h>
#include <viua/support/env.h>

namespace viua { namespace runtime { namespace imports {
constexpr char const* VIUA_LIBRARY_PATH = "VIUA_LIBRARY_PATH";

auto read_viua_library_path() -> std::vector<std::string>
{
    return viua::support::env::get_paths(VIUA_LIBRARY_PATH);
}

auto find_module(std::string const& module_name)
    -> std::optional<std::pair<Module_type, std::string>>
{
    auto const module_sep = std::regex{"::"};
    auto bytecode_file =
        std::regex_replace(module_name, module_sep, "/") + ".module";
    auto native_file = std::regex_replace(module_name, module_sep, "/") + ".so";

    for (auto const& each : read_viua_library_path()) {
        using viua::support::env::is_file;
        if (auto const candidate_path = each + '/' + bytecode_file;
            is_file(candidate_path)) {
            return std::pair<Module_type, std::string>{Module_type::Bytecode,
                                                       candidate_path};
        }
        if (auto const candidate_path = each + '/' + native_file;
            is_file(candidate_path)) {
            return std::pair<Module_type, std::string>{Module_type::Native,
                                                       candidate_path};
        }
    }
    return {};
}
}}}  // namespace viua::runtime::imports
