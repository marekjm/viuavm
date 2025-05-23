/*
 *  Copyright (C) 2025 Marek Marecki
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

#include <unistd.h>

#include <optional>
#include <print>

#include <viua/libexec/common.hh>
#include <viua/support/print.hh>


namespace viua {
namespace libexec {
auto maybe_show_info_and_exit(
    Common_options const& o) -> std::optional<int>
{
    if (o.show.version) {
        std::println("viua {} {}",
                     o.tool,
                     (o.verbosity ? VIUAVM_VERSION_FULL : VIUAVM_VERSION));
    }
    if (o.show.built_with) {
        std::println("compiler: {} {}", CXX, CXXVERSION);
        std::println("standard: {}", CXXSTD);
        std::println("preset:   {}", VIUAVM_CXX_PRESET);
        std::println("options:  {}", VIUAVM_CXX_OPTIONS);
    }
    if (o.show.version or o.show.built_with) {
        return 0;
    }

    if (o.show.help) {
        auto const man_page = std::format("viua-{}", o.tool);
        if (execlp("man", "man", "1", man_page.c_str(), nullptr) == -1) {
            viua::support::errorln(
                "man(1) page for {} not installed or not found", man_page);
            return 1;
        }
        return 0;
    }

    return std::nullopt;
}

Common_options::Common_options(
    std::string_view t)
    : tool{ t }
{}
}  // namespace libexec
}  // namespace viua
