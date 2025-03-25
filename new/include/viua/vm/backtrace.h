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

#ifndef VIUA_VM_BACKTRACE_H
#define VIUA_VM_BACKTRACE_H

#include <vector>

// FIXME Move Stack to stack.h and include that. We don't need the whole core
// definition, with I/O, processes, schedulers, etc.
#include <viua/vm/core.h>


namespace viua::vm::backtrace {
/*
 * Utility functions. Used in implementation of EBREAK, but also accessed by the
 * repl-debugger combo to dump backtraces and register dumps. Reuse makes
 * keeping the format consistent easier.
 */
auto print_backtrace(viua::vm::Stack const&,
                     std::optional<size_t> const = std::nullopt) -> void;
auto dump_registers(std::vector<viua::vm::Register> const&,
                    viua::vm::Process::atoms_map_type const&,
                    std::string_view const) -> void;
auto dump_memory(std::vector<viua::vm::Page> const&) -> void;
}

#endif
