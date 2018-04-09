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

#ifndef VIUA_ASSEMBLER_BACKEND_OP_ASSEMBLERS_H
#define VIUA_ASSEMBLER_BACKEND_OP_ASSEMBLERS_H

#include <vector>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>


namespace viua { namespace assembler { namespace backend {
namespace op_assemblers {
using Token      = viua::cg::lex::Token;
using Token_index = std::vector<viua::cg::lex::Token>::size_type;

using Single_register_op = Program& (Program::*)(int_op);
template<Single_register_op const op>
static auto assemble_single_register_op(Program& program,
        std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target = i + 1;

        (program.*op)(::assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
}

using Double_register_op = Program& (Program::*)(int_op, int_op);
template<Double_register_op const op>
static auto assemble_double_register_op(Program& program,
        std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target = i + 1;
        Token_index source = target + 2;

        (program.*op)(::assembler::operands::getint_with_rs_type(
                           ::assembler::operands::resolve_register(tokens.at(target)),
                           ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                       ::assembler::operands::getint_with_rs_type(
                           ::assembler::operands::resolve_register(tokens.at(source)),
                           ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
}

using Three_register_op = Program& (Program::*)(int_op, int_op, int_op);
template<Three_register_op const op>
static auto assemble_three_register_op(Program& program,
        std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        (program.*op)(::assembler::operands::getint_with_rs_type(
                          ::assembler::operands::resolve_register(tokens.at(target)),
                          ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                      ::assembler::operands::getint_with_rs_type(
                          ::assembler::operands::resolve_register(tokens.at(lhs)),
                          ::assembler::operands::resolve_rs_type(tokens.at(lhs + 1))),
                      ::assembler::operands::getint_with_rs_type(
                          ::assembler::operands::resolve_register(tokens.at(rhs)),
                          ::assembler::operands::resolve_rs_type(tokens.at(rhs + 1))));
}

using Capture_op = Program& (Program::*)(int_op, int_op, int_op);
template<Capture_op const op>
static auto assemble_capture_op(Program& program,
        std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target       = i + 1;
        Token_index inside_index = target + 2;
        Token_index source       = inside_index + 1;

        (program.*op)(::assembler::operands::getint_with_rs_type(
                              ::assembler::operands::resolve_register(tokens.at(target)),
                              ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                          ::assembler::operands::getint(
                              ::assembler::operands::resolve_register(tokens.at(inside_index))),
                          ::assembler::operands::getint_with_rs_type(
                              ::assembler::operands::resolve_register(tokens.at(source)),
                              ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
}

using Fn_ctor_op = Program& (Program::*)(int_op, std::string const&);
template<Fn_ctor_op const op>
static auto assemble_fn_ctor_op(Program& program,
        std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target = i + 1;
        Token_index source = target + 2;

        (program.*op)(::assembler::operands::getint_with_rs_type(
                              ::assembler::operands::resolve_register(tokens.at(target)),
                              ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                          tokens.at(source));
}

using ShiftOp = Program& (Program::*)(int_op, int_op, int_op);
template<const ShiftOp op>
static auto assemble_bit_shift_instruction(Program& program,
                                           const std::vector<Token>& tokens,
                                           const Token_index i) -> void {
    Token_index target = i + 1;
    Token_index lhs    = target + 2;
    Token_index rhs    = lhs + 2;

    int_op ret;
    if (tokens.at(target) == "void") {
        --lhs;
        --rhs;
        ret = ::assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target)));
    } else {
        ret = ::assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1)));
    }

    (program.*op)(ret,
                  ::assembler::operands::getint_with_rs_type(
                      ::assembler::operands::resolve_register(tokens.at(lhs)),
                      ::assembler::operands::resolve_rs_type(tokens.at(lhs + 1))),
                  ::assembler::operands::getint_with_rs_type(
                      ::assembler::operands::resolve_register(tokens.at(rhs)),
                      ::assembler::operands::resolve_rs_type(tokens.at(rhs + 1))));
}

using IncrementOp = Program& (Program::*)(int_op);
template<IncrementOp const op>
static auto assemble_increment_instruction(Program& program,
                                           std::vector<Token> const& tokens,
                                           Token_index const i) -> void {
    Token_index target = i + 1;

    (program.*op)(::assembler::operands::getint_with_rs_type(
        ::assembler::operands::resolve_register(tokens.at(target)),
        ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
}

using ArithmeticOp = Program& (Program::*)(int_op, int_op, int_op);
template<ArithmeticOp const op>
static auto assemble_arithmetic_instruction(Program& program,
                                            std::vector<Token> const& tokens,
                                            Token_index const i) -> void {
    Token_index target = i + 1;
    Token_index lhs    = target + 2;
    Token_index rhs    = lhs + 2;

    (program.*op)(::assembler::operands::getint_with_rs_type(
                      ::assembler::operands::resolve_register(tokens.at(target)),
                      ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                  ::assembler::operands::getint_with_rs_type(
                      ::assembler::operands::resolve_register(tokens.at(lhs)),
                      ::assembler::operands::resolve_rs_type(tokens.at(lhs + 1))),
                  ::assembler::operands::getint_with_rs_type(
                      ::assembler::operands::resolve_register(tokens.at(rhs)),
                      ::assembler::operands::resolve_rs_type(tokens.at(rhs + 1))));
}

auto assemble_op_integer(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_vinsert(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_vpop(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_bits(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_bitset(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_call(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_if(Program&, std::vector<Token> const&,
        Token_index const, viua::internals::types::bytecode_size const&,
        std::map<std::string, Token_index> const&) -> void;
auto assemble_op_jump(Program&, std::vector<Token> const&,
        Token_index const, viua::internals::types::bytecode_size const&,
        std::map<std::string, Token_index> const&) -> void;
auto assemble_op_structremove(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_msg(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_remove(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_float(Program&, std::vector<Token> const&,
        Token_index const) -> void;
auto assemble_op_frame(Program&, std::vector<Token> const&,
        Token_index const) -> void;
}}}}  // namespace viua::assembler::backend::op_assemblers


#endif
