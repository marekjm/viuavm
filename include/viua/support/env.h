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

#ifndef VIUA_SUPPORT_ENV_H
#define VIUA_SUPPORT_ENV_H

#include <sys/stat.h>

#include <string>
#include <vector>

namespace viua { namespace support { namespace env {
auto get_paths(std::string const&) -> std::vector<std::string>;
auto get_var(std::string const&) -> std::string;

auto is_file(std::string const&) -> bool;
}}}  // namespace viua::support::env

#endif
