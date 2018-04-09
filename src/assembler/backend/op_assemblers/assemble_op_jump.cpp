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

#include <tuple>
#include <viua/assembler/backend/op_assemblers.h>

namespace viua { namespace assembler { namespace backend {
namespace op_assemblers {
auto assemble_op_jump(Program& program, std::vector<Token> const& tokens,
        Token_index const i,
        viua::internals::types::bytecode_size const& instruction,
        std::map<std::string, Token_index> const& marks) -> void {
    /*  Jump instruction can be written in two forms:
     *
     *      * `jump <index>`
     *      * `jump :<marker>`
     *
     *  Assembler must distinguish between these two forms, and so it does.
     *  Here, we use a function from string support lib to determine
     *  if the jump is numeric, and thus an index, or
     *  a string - in which case we consider it a marker jump.
     *
     *  If it is a marker jump, assembler will look the marker up in a map
     * and if it is not found throw an exception about unrecognised marker
     * being used.
     */
    viua::internals::types::bytecode_size jump_target;
    enum JUMPTYPE jump_type;
    std::tie(jump_target, jump_type) =
        ::assembler::operands::resolve_jump(tokens.at(i + 1), marks, instruction);

    program.opjump(jump_target, jump_type);
}
}}}}  // namespace viua::assembler::backend::op_assemblers
