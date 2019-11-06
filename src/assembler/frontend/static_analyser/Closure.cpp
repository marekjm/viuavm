/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <viua/assembler/frontend/static_analyser.h>

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser {
auto Closure::define(Register const r, viua::cg::lex::Token const t) -> void
{
    defined_registers.insert_or_assign(
        r, std::pair<viua::cg::lex::Token, Register>(t, r));
}

Closure::Closure() : name("") {}
Closure::Closure(std::string n) : name(n) {}
}}}}  // namespace viua::assembler::frontend::static_analyser
