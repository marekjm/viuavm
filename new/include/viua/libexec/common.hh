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

#ifndef VIUA_LIBEXEC_COMMON_HH
#define VIUA_LIBEXEC_COMMON_HH

#include <stdint.h>

#include <optional>
#include <string_view>


namespace viua {
namespace libexec {
struct Common_options {
    std::string_view tool;

    size_t verbosity{ 0 };
    struct {
        bool version{ false };
        bool built_with{ false };
        bool help{ false };
    } show;

    explicit Common_options(std::string_view);
};

auto maybe_show_info_and_exit(Common_options const&) -> std::optional<int>;
}  // namespace libexec
}  // namespace viua

#endif
