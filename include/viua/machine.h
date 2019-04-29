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

#ifndef VIUA_MACHINE_H
#define VIUA_MACHINE_H

#pragma once

#define VIUA_VM_DEBUG_LOG 0


constexpr auto ENTRY_FUNCTION_NAME[[maybe_unused]] = "__entry";
constexpr auto VIUA_MAGIC_NUMBER[[maybe_unused]] = "VIUA";

using Viua_binary_type = char;

enum Viua_module_type {
    VIUA_LINKABLE = Viua_binary_type{'L'},
    VIUA_EXECUTABLE = Viua_binary_type{'E'},
};


#endif
