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
auto assemble_op_if(Program& program,
                    std::vector<Token> const& tokens,
                    Token_index const i,
                    viua::internals::types::bytecode_size const& instruction,
                    std::map<std::string, Token_index> const& marks) -> void {
    /*  If branch is given three operands, it means its full, three-operands
     * form is being used. Otherwise, it is short, two-operands form
     * instruction and assembler should fill third operand accordingly.
     *
     *  In case of short-form `branch` instruction:
     *
     *      * first operand is index of the register to check,
     *      * second operand is the address to which to jump if register is
     * true,
     *      * third operand is assumed to be the *next instruction*, i.e.
     * instruction after the branch instruction,
     *
     *  In full (with three operands) form of `branch` instruction:
     *
     *      * third operands is the address to which to jump if register is
     * false,
     */
    Token condition = tokens.at(i + 1);
    Token if_true   = tokens.at(i + 3);
    Token if_false  = tokens.at(i + 4);

    viua::internals::types::bytecode_size addrt_target, addrf_target;
    enum JUMPTYPE addrt_jump_type, addrf_jump_type;
    std::tie(addrt_target, addrt_jump_type) =
        ::assembler::operands::resolve_jump(
            tokens.at(i + 3), marks, instruction);
    if (if_false != "\n") {
        std::tie(addrf_target, addrf_jump_type) =
            ::assembler::operands::resolve_jump(
                tokens.at(i + 4), marks, instruction);
    } else {
        addrf_jump_type = JMP_RELATIVE;
        addrf_target    = instruction + 1;
    }

    program.opif(::assembler::operands::getint_with_rs_type(
                     ::assembler::operands::resolve_register(condition),
                     ::assembler::operands::resolve_rs_type(tokens.at(i + 2))),
                 addrt_target,
                 addrt_jump_type,
                 addrf_target,
                 addrf_jump_type);
}
}}}}  // namespace viua::assembler::backend::op_assemblers
