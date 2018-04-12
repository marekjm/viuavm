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

#include <viua/assembler/backend/op_assemblers.h>

namespace viua { namespace assembler { namespace backend {
namespace op_assemblers {
auto assemble_op_vector(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target           = i + 1;
        Token_index pack_range_start = target + 2;
        Token_index pack_range_count = pack_range_start + 2;

        program.opvector(::assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(target)),
                             ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                         ::assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(pack_range_start)),
                             ::assembler::operands::resolve_rs_type(tokens.at(pack_range_start + 1))),
                         ::assembler::operands::getint(
                             ::assembler::operands::resolve_register(tokens.at(pack_range_count))));
}
}}}}  // namespace viua::assembler::backend::op_assemblers
