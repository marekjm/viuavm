/*
 *  Copyright (C) 2016 Marek Marecki
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

#ifndef VIUA_CG_TOOLS_H
#define VIUA_CG_TOOLS_H

#pragma once

#include <cstdint>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>


namespace viua { namespace cg { namespace tools {
auto calculate_bytecode_size_of_first_n_instructions2(
    std::vector<viua::cg::lex::Token> const& tokens,
    std::remove_reference<decltype(tokens)>::type::size_type const limit)
    -> viua::internals::types::bytecode_size;
auto calculate_bytecode_size2(std::vector<viua::cg::lex::Token> const&)
    -> viua::internals::types::bytecode_size;
}}}  // namespace viua::cg::tools


#endif
