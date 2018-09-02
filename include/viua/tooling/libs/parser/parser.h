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

#ifndef VIUA_TOOLING_LIBS_PARSER_PARSER_H
#define VIUA_TOOLING_LIBS_PARSER_PARSER_H

#include <memory>
#include <string>
#include <vector>
#include <viua/bytecode/operand_types.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/tooling/libs/lexer/tokenise.h>

namespace viua {
namespace tooling {
namespace libs {
namespace parser {

enum class Fragment_type {
    Signature_directive,
    Block_signature_directive,
    Closure_head,
    Instruction,
};

/*
 * Fragment represents a single element of the program; a function name,
 * an attribute list, a register address, an instruction, etc.
 * Raw tokens are reduced to fragments.
 */
class Fragment {
    std::vector<viua::tooling::libs::lexer::Token> tokens;
    Fragment_type const fragment_type;

  public:
    auto type() const -> Fragment_type;
    auto add(viua::tooling::libs::lexer::Token) -> void;

    Fragment(Fragment_type const);
};

struct Signature_directive : public Fragment {
    std::string const function_name;
    uint64_t const arity;

    Signature_directive(std::string, uint64_t const);
};

struct Block_signature_directive : public Fragment {
    std::string const block_name;

    Block_signature_directive(std::string);
};

struct Closure_head : public Fragment {
    std::string const function_name;
    uint64_t const arity;
    std::set<std::string> const attributes;

    Closure_head(std::string, uint64_t const, std::set<std::string>);
};

enum class Operand_type {
    Register_address,
};

class Operand {
    std::vector<viua::tooling::libs::lexer::Token> tokens;
    Operand_type const operand_type;

  public:
    auto type() const -> Operand_type;
    auto add(viua::tooling::libs::lexer::Token) -> void;

    Operand(Operand_type const);
};

struct Register_address : public Operand {
    viua::internals::types::register_index const index;
    viua::internals::Register_sets const register_set;
    viua::internals::Access_specifier const access;

    Register_address(
        viua::internals::types::register_index const
        , viua::internals::Register_sets const
        , viua::internals::Access_specifier const
    );
};

struct Instruction : public Fragment {
    OPCODE const opcode;
    std::vector<std::unique_ptr<Operand>> operands;

    Instruction(OPCODE const);
};

auto parse(std::vector<viua::tooling::libs::lexer::Token> const&) -> std::vector<std::unique_ptr<Fragment>>;

}}}}

#endif
