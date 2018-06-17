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
auto assemble_op_call(Program& program,
                      std::vector<Token> const& tokens,
                      Token_index const i) -> void {
    /** Full form of call instruction has two operands: function name and
     *  return value register index.
     *  If call is given only one operand it means it is the function name
     * and returned value is discarded. To explicitly state that return
     * value should be discarderd put `void` as return register index.
     */
    /** Why is the function supplied as a *string* and
     *  not direct instruction pointer?
     *  That would be faster - c'mon couldn't assembler just calculate
     * offsets and insert them?
     *
     *  Nope.
     *
     *  Yes, it *would* be faster if calls were just precalculated jumps.
     *  However, by them being strings we get plenty of flexibility,
     * good-quality stack traces, and a place to put plenty of debugging
     * info. All that at a cost of just one map lookup; the overhead is
     * minimal and gains are big. What's not to love?
     *
     *  Of course, you, my dear reader, are free to take this code (it's GPL
     * after all!) and modify it to suit your particular needs - in that
     * case that would be calculating call jumps at compile time and
     * exchanging CALL instructions with JUMP instructions.
     *
     *  Good luck with debugging your code, then.
     */
    Token_index target = i + 1;
    Token_index fn     = target + 2;

    int_op ret;
    if (tokens.at(target) == "void") {
        --fn;
        ret = ::assembler::operands::getint(
            ::assembler::operands::resolve_register(tokens.at(target)));
    } else {
        ret = ::assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1)));
    }

    if (tokens.at(fn).str().at(0) == '*' or tokens.at(fn).str().at(0) == '%') {
        program.opcall(
            ret,
            ::assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(fn)),
                ::assembler::operands::resolve_rs_type(tokens.at(fn + 1))));
    } else {
        program.opcall(ret, tokens.at(fn));
    }
}
}}}}  // namespace viua::assembler::backend::op_assemblers
